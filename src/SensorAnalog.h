#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorAnalog : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;
    float Coefficient;

    enum ESensor {
        Analog
    };
    
    TSensorAnalog(uint8_t pin, TActor* owner, StringBuf name = "some", StringBuf value = "analog", float coefficient = 1.0)
        : Owner(owner)
        , Coefficient(coefficient)
        , PinValue(pin, INPUT)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Analog].Name = value;
    }

protected:
    TAnalogPin PinValue;

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

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        Sensor.Values[ESensor::Analog].Value = PinValue.GetValue() * Coefficient;
        Sensor.Updated = context.Now;
        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Analog]));
        }
		event->NotBefore = context.Now + Env::SensorsPeriod;
		context.Resend(this, event.Release());
    }
};

}