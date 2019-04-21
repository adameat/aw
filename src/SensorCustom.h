#pragma once

#include "aw.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorCustom : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;

    enum ESensor {
        Value
    };

    TSensorCustom(TActor* owner, StringBuf sensor = "sensor", StringBuf value = "value")
        : Owner(owner) {
        Sensor.Name = sensor;
        Sensor.Values[ESensor::Value].Name = value;
    }

    TOptionalValue& GetValue() {
        return Sensor.Values[ESensor::Value].Value;
    }

//protected:
    void OnEvent(TEventPtr, const TActorContext&) override {}

    void SetValue(double value, const AW::TActorContext& context) {
        Sensor.Values[ESensor::Value].Value.SetValue(value);
        Sensor.Updated = context.Now;
        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Value]));
        }
    }
};

}