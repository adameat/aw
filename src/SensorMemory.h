#pragma once

#include "aw.h"

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;
extern "C" char* sbrk(int incr);

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorMemory : public TActor, public TSensorSource {
public:
    TActor* Owner;
    TSensorValueULong Free;

    TSensorMemory(TActor* owner, StringBuf name = "memory")
        : Owner(owner)
    {
        Name = name;
        Free.Name = "free";
    }

protected:
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        context.Send(this, this, new AW::TEventReceive(context.Now + Env::SensorsPeriod));
    }

#if defined(ARDUINO_ARCH_SAMD)
    static uint32_t GetFreeMemory() {
        uint16_t freeMemory;
        return ((uint32_t)&freeMemory) - (uint32_t)reinterpret_cast<char*>(sbrk(0));
    }
#elif defined(ARDUINO_ARCH_AVR)
    static uint16_t GetFreeMemory() {
        uint16_t freeMemory;
        if ((int)__brkval == 0)
            return ((uint16_t)&freeMemory) - ((uint16_t)&__bss_end);
        else
            return ((uint16_t)&freeMemory) - ((uint16_t)__brkval);
    }
#endif

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        Free.Value = GetFreeMemory();
        Updated = context.Now;
        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(*this, Free));
        }
        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());
    }
};

}