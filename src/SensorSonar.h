#pragma once

#include "ArduinoWorkflow.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorSonar : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;

    enum ESensor {
        Distance
    };

    TSensorSonar(uint8_t triggerPin, uint8_t echoPin, TActor* owner, StringBuf name = "sonar")
        : Owner(owner)
        , PinTrigger(triggerPin, OUTPUT)
        , PinEcho(echoPin, INPUT)
    {
        Sensor.Name = name;
        Sensor.Values[ESensor::Distance].Name = "distance";
    }

protected:
    TDigitalPin PinTrigger;
    TDigitalPin PinEcho;
    volatile unsigned long StartTime = 0;
    volatile unsigned long EndTime = 0;

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
        This() = this;
        attachInterrupt(digitalPinToInterrupt(PinEcho.GetPin()), StaticInterrupt, CHANGE);
        context.Send(this, this, new TEventReceive());
    }

    void Send(const TActorContext& context) {
        auto value = EndTime - StartTime;
        if (value < 100000) {
            Sensor.Values[ESensor::Distance].Value = double(value) / 5800;
            Sensor.Updated = context.Now;
            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new TEventSensorData(Sensor, Sensor.Values[ESensor::Distance]));
            }
        }
    }

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        if (EndTime > StartTime) {
            Send(context);
        }
        StartTime = 0;
        EndTime = 0;
        PinTrigger = true;
        delayMicroseconds(10);
        PinTrigger = false;
        delay(10);
        if (EndTime > StartTime) {
            Send(context);
            StartTime = 0;
            EndTime = 0;
        }

        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());
    }

    void Interrupt() {
        auto time = micros();
        bool value = PinEcho;
        if (value) {
            StartTime = time;
        } else {
            EndTime = time;
        }
    }

    static TSensorSonar*& This() { static TSensorSonar* _this; return _this; }

    static void StaticInterrupt() {
        This()->Interrupt();
    }
};


}
