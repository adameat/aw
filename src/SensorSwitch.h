#pragma once

#include "aw.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorSwitch : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;
    uint32_t Value = 0;

    enum ESensor {
        Switch,
    };
    
    TSensorSwitch(uint8_t pin, TActor* owner, StringBuf name = "switch")
        : Owner(owner)
        , PinValue(pin, INPUT_PULLUP)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Switch].Name = "switch";

    }

protected:
    static constexpr TTime Resolution = TTime::MilliSeconds(100);
    TDigitalPin PinValue;
    
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
        Value = PinValue ? 0 : 1;
        context.Send(this, this, new TEventReceive());
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        Value = PinValue ? 0 : 1;
        if (Value != Sensor.Values[ESensor::Switch].Value.GetValue()) {
            Sensor.Values[ESensor::Switch].Value.SetValue(Value);
            Sensor.Updated = context.Now;
            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::Switch]));
            }
        }
        event->NotBefore = context.Now + Resolution;
        context.Resend(this, event.Release());
    }
};

}
