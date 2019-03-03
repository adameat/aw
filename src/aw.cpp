#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include "aw.h"

namespace AW {

TActorContext::TActorContext(TActorLib& actorLib)
    : ActorLib(actorLib)
    , Now(TTime::Now()) {}

void TActorContext::Send(TActor* sender, TActor* recipient, TEventPtr event) const {
    event->Sender = sender;
    recipient->OnSend(event, *this);
}

void TActorContext::SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) const {
    event->Sender = sender;
    ActorLib.SendImmediate(recipient, event);
}

void TActorContext::Resend(TActor* recipient, TEventPtr event) const {
    ActorLib.Resend(recipient, event);
}

void TActorContext::ResendImmediate(TActor* recipient, TEventPtr event) const {
    ActorLib.ResendImmediate(recipient, event);
}

void TActorContext::ResendAfter(TActor* recipient, TEventPtr event, TTime time) const {
    event->NotBefore = Now + time;
    Resend(recipient, event);
}

TActorLib::TActorLib() {
    Watchdog.enable(8000);
}

void TActorLib::Register(TActor* actor) {
    TActor* itActor = Actors;
    if (itActor == nullptr) {
        Actors = actor;
    } else {
        while (itActor->NextActor != nullptr) {
            itActor = itActor->NextActor;
        }
        itActor->NextActor = actor;
    }
    TEventPtr bootstrapEvent = new TEventBootstrap;
    Send(actor, bootstrapEvent);
}

void TActorLib::Run() {
    TActorContext context(*this);
    context.Now += SleepTime;
    TActor* itActor = Actors;
    TTime minSleep = TTime::Max();
    while (itActor != nullptr) {
        Watchdog.reset();
        auto& events(itActor->Events);
        auto end(events.end());
        auto itEvent = events.begin();
        while (itEvent != events.end() && itEvent != end) {
            TEventPtr event = events.pop_value(itEvent);
            if (context.Now < event->NotBefore) {
                TTime sleep = event->NotBefore - context.Now;
                if (minSleep > sleep) {
                    minSleep = sleep;
                }
                auto itEnd = events.push_back(event);
                if (end == events.end()) {
                    end = itEnd;
                }
            } else {
                minSleep = TTime::Zero();
                TTime start = TTime::Now();
                itActor->OnEvent(event, context);
                TTime spent = TTime::Now() - start;
                itActor->BusyTime += spent;
                BusyTime += spent;
                if (itEvent != events.begin())
                    break;
            }
        }
        itActor = itActor->NextActor;
    }
    if (Sleeping || (minSleep.IsValid() && minSleep >= TTime::Seconds(1))) {
        //AVR
        //LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
        //SAMD
        //LowPower.deepSleep(1999);
        auto slept = Watchdog.sleep(1000);
        SleepTime += TTime::MilliSeconds(slept);
    }
}

void TActorLib::Send(TActor* recipient, TEventPtr event) {
    recipient->Events.push_back(event);
}

void TActorLib::SendImmediate(TActor* recipient, TEventPtr event) {
    recipient->Events.push_front(event);
}

void TActorLib::SendSync(TActor* recipient, TEventPtr event) {
    TActorContext context(*this);
    context.Now += SleepTime;
    Watchdog.reset();
    TTime start = TTime::Now();
    recipient->OnEvent(event, context);
    TTime spent = TTime::Now() - start;
    recipient->BusyTime += spent;
    BusyTime += spent;
}

void TActorLib::Resend(TActor* recipient, TEventPtr event) {
    recipient->Events.push_back(event);
}

void TActorLib::ResendImmediate(TActor* recipient, TEventPtr event) {
    recipient->Events.push_front(event);
}

String TTime::AsString() const {
    return String(Value);
}

char String::ConversionBuffer[32];

void Reset() {
#ifdef ARDUINO_ARCH_AVR
	void(*reset)() = nullptr;
	reset();
#endif
#ifdef ARDUINO_ARCH_SAMD
    NVIC_SystemReset();
#endif
#ifdef ARDUINO_ARCH_STM32F1
    nvic_sys_reset();
#endif
}

#ifdef ARDUINO_ARCH_SAMD
void HardFault_Handler() {
    Reset();
}

void Reset_Handler() {
    Reset();
}
#endif

TEventData::TEventData(const String& data)
    : Data(data)
{}

void TActor::OnSend(TEventPtr event, const TActorContext& context) {
    context.ActorLib.Send(this, event);
}

void TActor::PurgeEvents(TEventID eventId) {
    for (auto it = Events.begin(); it != Events.end(); ) {
        if (it.Get()->EventID == eventId) {
            it = Events.erase(it);
        } else {
            ++it;
        }
    }
}

}