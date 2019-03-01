#pragma once

#include "ArduinoWorkflow.h"
//#include <OneWire.h>

#define REQUIRESALARMS false

#include <DallasTemperature.h>

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorDS18B20 : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;

    enum ESensor {
        Temperature,
    };

    TSensorDS18B20(uint8_t oneWirePin, TActor* owner, StringBuf name = "ds18b20")
        : Owner(owner)
        , OneWireBus(oneWirePin)
        , DS(&OneWireBus)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Temperature].Name = "temperature";
    }

protected:
    OneWire OneWireBus;
    DallasTemperature DS;
    TTime ConversionPeriod;
    bool Requested = false;

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
        DS.begin();
        DS.setWaitForConversion(false);
        ConversionPeriod = TTime::MilliSeconds(DS.millisToWaitForConversion(DS.getResolution()));
        context.Send(this, this, new AW::TEventReceive());
        if (Env::Diagnostics) {
            context.Send(this, Owner, new TEventSensorMessage(Sensor, StringStream() << "DS18B20"));
        }
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        if (Requested && DS.isConversionComplete()) {
            Requested = false;
            auto temp = DS.getTempCByIndex(0);
            if (temp != DEVICE_DISCONNECTED_C) {
                Sensor.Values[ESensor::Temperature].Value = temp;
                Sensor.Updated = context.Now;
                if (Env::SensorsSendValues) {
                    context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::Temperature]));
                }
            }
            event->NotBefore = context.Now + Env::SensorsPeriod;
        } else {
            Requested = true;
            DS.requestTemperatures();
            event->NotBefore = context.Now + ConversionPeriod;
        }
        context.Resend(this, event.Release());
    }
};

}