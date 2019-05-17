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
    recipient->OnSend(Move(event), *this);
}

void TActorContext::SendAfter(TActor* sender, TActor* recipient, TEventPtr event, TTime time) const {
    event->NotBefore = Now + time;
    Send(sender, recipient, Move(event));
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
#ifndef _DEBUG_SLEEP
    auto slept = Watchdog.sleep(10); // it will reset here first time after flash... don't know, why
    SleepTime += TTime::MilliSeconds(slept);
#endif
#ifndef _DEBUG_WATCHDOG
    Watchdog.enable(WatchdogTimeout.MilliSeconds());
#endif
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
    TActor* itActor = Actors;
    TTime nextEvent = TTime::Max();
    TTime now;
    while (itActor != nullptr) {
#ifndef _DEBUG_WATCHDOG
        Watchdog.reset();
#endif
        auto& events(itActor->Events);
        auto end(events.end());
        auto itEvent = events.begin();
        events.size();
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
                events.size();
                now = TTime::Now();
                TTime spent = now - start;
                itActor->BusyTime += spent;
                BusyTime += spent;
                if (itEvent != events.begin())
                    break;
            }
        }
        events.size();
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
        if (minSleep > MaxSleepPeriod) {
            minSleep = MaxSleepPeriod;
        }
        if (minSleep >= MinSleepPeriod) {
            auto sleep = minSleep.MilliSeconds();
#ifndef _DEBUG_WATCHDOG
            Watchdog.disable();
#endif
#ifdef _DEBUG_SLEEP
            delay(sleep);
#else
            auto slept = Watchdog.sleep(sleep);
            SleepTime += TTime::MilliSeconds(slept);
#endif
#ifndef _DEBUG_WATCHDOG
            Watchdog.enable(WatchdogTimeout.MilliSeconds());
#endif
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
#ifndef _DEBUG_WATCHDOG
    Watchdog.reset();
#endif
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
#ifdef ARDUINO
static volatile char LastResetReason[16] __attribute__((section(".noinit")));
#else
static volatile char LastResetReason[16];
#endif

void DefaultReset(StringBuf reason) {
    memcpy(const_cast<char*>(LastResetReason), reason.begin(), min(reason.size(), (StringBuf::size_type)(sizeof(LastResetReason) - 1)));
#ifdef _DEBUG_HANG_ON_RESET
#ifndef _DEBUG_WATCHDOG
    Watchdog.disable();
#endif
    for (;;) {
        /*if (!reason.empty()) {
            Serial.write(reason.data(), reason.size());
        } else {
            Serial.print("reset");
        }
        Serial.print(" ");
        Serial.flush();
        delay(1000);*/
        yield();
    }
#endif
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

/*void TWire::Write(const void* data, int size) {
    for (const uint8_t* d = (const uint8_t*)data + size - 1; d >= data; --d) {
        Wire.write(*d);
    }
}

void TWire::Read(void* data, int size) {
    for (uint8_t* d = (uint8_t*)data + size - 1; d >= data; --d) {
        *d = Wire.read();
    }
}

void TWire::ReadLE(void* data, int size) {
    for (uint8_t* d = (uint8_t*)data; d < (uint8_t*)data + size; ++d) {
        *d = Wire.read();
    }
}

bool TWire::ReadValue(uint8_t addr, uint8_t reg, void* val, int size) {
    BeginTransmission(addr);
    Write(reg);
    if (!EndTransmission())
        return false;
    if (RequestFrom(addr, size) != size)
        return false;
    Read(val, size);
    return true;
}

bool TWire::ReadValueLE(uint8_t addr, uint8_t reg, void* val, int size) {
    BeginTransmission(addr);
    Write(reg);
    if (!EndTransmission())
        return false;
    if (RequestFrom(addr, size) != size)
        return false;
    ReadLE(val, size);
    return true;
}

bool TWire::WriteValue(uint8_t addr, uint8_t reg, const void* val, int size) {
    BeginTransmission(addr);
    Write(reg);
    Write(val, size);
    return EndTransmission();
}*/

constexpr TTime TActorLib::MinSleepPeriod;
constexpr TTime TActorLib::MaxSleepPeriod;
constexpr TTime TDefaultEnvironment::WarmupPeriod;

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
