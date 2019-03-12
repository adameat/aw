#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include "aw.h"

//extern Uart ConsoleSerial;

namespace AW {

TActorContext::TActorContext(TActorLib& actorLib)
    : ActorLib(actorLib)
    , Now(TTime::Now()) {}

void TActorContext::Send(TActor* sender, TActor* recipient, TEventPtr event) const {
    event->Sender = sender;
    recipient->OnSend(Move(event), *this);
}

void TActorContext::SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) const {
    event->Sender = sender;
    ActorLib.SendImmediate(recipient, Move(event));
}

void TActorContext::Resend(TActor* recipient, TEventPtr event) const {
    ActorLib.Resend(recipient, Move(event));
}

void TActorContext::ResendImmediate(TActor* recipient, TEventPtr event) const {
    ActorLib.ResendImmediate(recipient, Move(event));
}

void TActorContext::ResendAfter(TActor* recipient, TEventPtr event, TTime time) const {
    event->NotBefore = Now + time;
    Resend(recipient, Move(event));
}

TActorLib::TActorLib() {
    //Watchdog.sleep(10); // it will reset here first time after flash... don't know, why
    Watchdog.enable(WatchdogTimeout.MilliSeconds());
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
    Send(actor, Move(bootstrapEvent));
}

void TActorLib::Run() {
    TActorContext context(*this);
    //context.Now += SleepTime;
    TActor* itActor = Actors;
    //TTime minSleep = TTime::Max();
    TTime nextEvent = TTime::Max();
    TTime now;
    while (itActor != nullptr) {
        Watchdog.reset();
        auto& events(itActor->Events);
        auto end(events.end());
        auto itEvent = events.begin();
        while (itEvent != events.end() && itEvent != end) {
            TEventPtr event = events.pop_value(itEvent);
            TTime start = now = TTime::Now();
            context.Now = start + SleepTime;
            if (context.Now < event->NotBefore) {
                if (nextEvent > event->NotBefore) {
                    nextEvent = event->NotBefore;
                }
                auto itEnd = events.push_back(Move(event));
                if (end == events.end()) {
                    end = itEnd;
                }
            } else {
                nextEvent = TTime::Zero();
                itActor->OnEvent(Move(event), context);
                now = TTime::Now();
                TTime spent = now - start;
                itActor->BusyTime += spent;
                BusyTime += spent;
                if (itEvent != events.begin())
                    break;
            }
        }
        itActor = itActor->NextActor;
    }
    if (nextEvent != TTime::Zero()) {
        now += SleepTime;
        TTime minSleep;
        if (nextEvent > now) {
            minSleep = nextEvent - now;
        }
        if (Sleeping) {
            if (minSleep < MinSleepPeriod) {
                minSleep = MinSleepPeriod;
            }
        }
        if (minSleep >= MinSleepPeriod) {
            auto sleep = minSleep.MilliSeconds();

            //ConsoleSerial.print(sleep);
            //ConsoleSerial.print("->");

            Watchdog.disable();
            //auto slept = Watchdog.sleep(sleep);
            delay(sleep);
            Watchdog.enable(WatchdogTimeout.MilliSeconds());

            //ConsoleSerial.println(slept);

            //SleepTime += TTime::MilliSeconds(slept);
        }
    }
}

void TActorLib::Send(TActor* recipient, TEventPtr event) {
    recipient->Events.push_back(Move(event));
}

void TActorLib::SendImmediate(TActor* recipient, TEventPtr event) {
    recipient->Events.push_front(Move(event));
}

void TActorLib::SendSync(TActor* recipient, TEventPtr event) {
    TActorContext context(*this);
    context.Now += SleepTime;
    Watchdog.reset();
    TTime start = TTime::Now();
    recipient->OnEvent(Move(event), context);
    TTime spent = TTime::Now() - start;
    recipient->BusyTime += spent;
    BusyTime += spent;
}

void TActorLib::Resend(TActor* recipient, TEventPtr event) {
    recipient->Events.push_back(Move(event));
}

void TActorLib::ResendImmediate(TActor* recipient, TEventPtr event) {
    recipient->Events.push_front(Move(event));
}

void TActorLib::Sleep() {
    Sleeping = true;
    /*TActor* itActor = Actors;
    while (itActor != nullptr) {
        SendSync(itActor, new TEventSleep());
        itActor = itActor->NextActor;
    }*/
}

void TActorLib::WakeUp() {
    /*TActor* itActor = Actors;
    while (itActor != nullptr) {
        SendSync(itActor, new TEventWakeUp());
        itActor = itActor->NextActor;
    }*/
    Sleeping = false;
}

String TTime::AsString() const {
    return String(Value);
}

char String::ConversionBuffer[32];
static volatile char LastResetReason[8] __attribute__((section(".noinit")));

void DefaultReset(StringBuf reason) {
    memcpy(const_cast<char*>(LastResetReason), reason.begin(), min(reason.size(), sizeof(LastResetReason) - 1));
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

StringBuf GetLastResetReason() {
    return const_cast<const char*>(LastResetReason);
}

void(*Reset)(StringBuf reason) = &DefaultReset;

TEventData::TEventData(const String& data)
    : Data(data)
{}

void TActor::OnSend(TEventPtr event, const TActorContext& context) {
    context.ActorLib.Send(this, Move(event));
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

constexpr TTime TActorLib::MinSleepPeriod;

}

#ifdef ARDUINO_ARCH_SAMD
void HardFault_Handler() {
    AW::Reset("HFAULT");
}

/*void Reset_Handler() {
    AW::Reset("Reset");
}*/

void NMI_Handler() {
    AW::Reset("NMI");
}

void SVC_Handler() {
    AW::Reset("SVC");
}

void PendSV_Handler() {
    AW::Reset("PEND-SV");
}

void PM_Handler() {
    AW::Reset("PM");
}

void SYSCTRL_Handler() {
    AW::Reset("SYSCTRL");
}

/*void WDT_Handler() {
    AW::Reset("WDT");
}*/

void RTC_Handler() {
    AW::Reset("RTC");
}

void EIC_Handler() {
    AW::Reset("EIC");
}

void NVMCTRL_Handler() {
    AW::Reset("NVMCTRL");
}

void DMAC_Handler() {
    AW::Reset("DMAC");
}

void EVSYS_Handler() {
    AW::Reset("EVSYS");
}

void ADC_Handler() {
    AW::Reset("ADC");
}

void AC_Handler() {
    AW::Reset("AC");
}

void DAC_Handler() {
    AW::Reset("DAC");
}

void PTC_Handler() {
    AW::Reset("PTC");
}

void I2S_Handler() {
    AW::Reset("I2S");
}

#endif
