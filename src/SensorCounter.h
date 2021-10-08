#pragma once

#include "aw.h"

namespace AW {

template <uint8_t Pin, typename Env = TDefaultEnvironment>
class TSensorInterruptCounter : public TActor, public TSensorSource {
public:
    TActor* Owner;
    TSensorValueULong Sensor;
    /*volatile */uint32_t Value = 0;

    TSensorInterruptCounter(TActor* owner, StringBuf name = "counter")
        : Owner(owner)
        , PinValue(INPUT_PULLUP)
    {
        Name = name;
        Sensor.Name = "counter";

    }

protected:
    static constexpr TTime MinDelayLow = TTime::MilliSeconds(500);
    static constexpr TTime MinDelayHigh = TTime::MilliSeconds(500);
    TDigitalPin<Pin> PinValue;
    /*volatile */bool LastValue;
    /*volatile */TTime LastTime;
    /*volatile */TTime Low;
    /*volatile */TTime High;
    
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        This() = this;
        LastValue = PinValue;
        LastTime = context.Now;
        attachInterrupt(digitalPinToInterrupt(PinValue.GetPin()), StaticInterrupt, CHANGE);
        context.Send(this, this, new TEventReceive());
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        if (Value != Sensor.GetValue()) {
            Sensor.SetValue(Value);
            Updated = context.Now;
            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new TEventSensorData(*this, Sensor));
            }
        }
        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());
    }

    void Interrupt() {
        bool value = PinValue;
        if (value != LastValue) {
            TTime now = TTime::Now();
            TTime delay = now - LastTime;
            switch (value) {
            case false:
                if (delay < MinDelayLow) {
                    LastValue = value;
                    return;
                }
                High = now;
                break;
            case true:
                if (delay < MinDelayHigh) {
                    LastValue = value;
                    return;
                }
                Low = now;
                ++Value;
                break;
            }
            LastTime = now;
            LastValue = value;
        }
    }

    static TSensorInterruptCounter*& This() { static TSensorInterruptCounter* _this; return _this; }

    static void StaticInterrupt() {
        This()->Interrupt();
    }
};

template <uint8_t Pin, typename Env = TDefaultEnvironment>
class TSensorPollingCounter : public TActor, public TSensorSource {
public:
    TActor * Owner;
    TSensorValueULong Sensor;
    uint32_t Value = 0;

    TSensorPollingCounter(TActor* owner, StringBuf name = "counter")
        : Owner(owner)
        , PinValue(INPUT_PULLUP)
    {
        Name = name;
        Sensor.Name = "counter";
    }

protected:
    static constexpr TTime MinDelayLow = TTime::MilliSeconds(500);
    static constexpr TTime MinDelayHigh = TTime::MilliSeconds(500);
    static constexpr TTime Resolution = TTime::MilliSeconds(50);
    TDigitalPin<Pin> PinValue;
    bool LastValue;
    TTime LastTime;
    TTime Low;
    TTime High;

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
        LastValue = PinValue;
        LastTime = context.Now;
        context.Send(this, this, new TEventReceive());
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        bool value = PinValue;
        if (value != LastValue) {
            TTime now = context.Now;
            TTime delay = now - LastTime;
            switch ((int)value) {
            case false:
                if (delay < MinDelayLow) {
                    LastValue = value;
                    break;
                }
                High = now;
                break;
            case true:
                if (delay < MinDelayHigh) {
                    LastValue = value;
                    break;
                }
                Low = now;
                ++Value;
                break;
            }
            LastTime = now;
            LastValue = value;
        }
        if (Value != Sensor.GetValue()) {
            Sensor.SetValue(Value);
            Updated = context.Now;
            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new TEventSensorData(*this, Sensor));
            }
        }
        event->NotBefore = context.Now + Resolution;
        context.Resend(this, event.Release());
    }
};


}
