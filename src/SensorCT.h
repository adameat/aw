#pragma once

#include "ArduinoWorkflow.h"
#include <EmonLib.h>

namespace AW {

template <uint8_t Pin, typename Env = TDefaultEnvironment>
class TSensorCT : public TActor {
public:
    TActor* Owner;
    TSensor<1> Sensor;

    enum ESensor {
        Current
    };

    TSensorCT(TActor* owner, StringBuf name = "ct")
        : Owner(owner)
    {
		EMon.current(Pin, 111.1);
        Sensor.Name = name;
        Sensor.Values[ESensor::Current].Name = "current";
    }

	void Calibrate(double calibration, unsigned int samples) {
		EMon.current(Pin, calibration);
		Samples = samples;
	}

protected:
    EnergyMonitor EMon;
	unsigned int Samples = 1480;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        context.Send(this, this, new AW::TEventReceive(context.Now + Env::SensorsPeriod));
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        Sensor.Values[ESensor::Current].Value = EMon.calcIrms(Samples);
        Sensor.Updated = context.Now;
		if (Env::SensorsSendValues) {
			context.Send(this, Owner, new AW::TEventSensorData(Sensor, Sensor.Values[ESensor::Current]));
		}
        //context.ResendAfter(this, event.Release(), Env::SensorsPeriod);
		event->NotBefore = context.Now + Env::SensorsPeriod;
		context.Resend(this, event.Release());
    }
};

}