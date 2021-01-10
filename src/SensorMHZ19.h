#pragma once

#include "aw.h"

namespace AW {

template <typename SerialType, typename Env = TDefaultEnvironment>
class TSensorMHZ19 : public TActor, public TSensorSource {
public:
    uint8_t Address;
    uint16_t ChipId = 0;
    uint16_t ConfigValue;
    SerialType Serial;
    TActor* Owner;
    TSensorValueFloat CO2;
    TSensorValueFloat Temperature;
    TSensorValueULong Calibrations;
    TTime LastCO2Seen;

    TSensorMHZ19(TActor* owner, String name = "mhz19")
        : Owner(owner)
    {
        Name = name;
        CO2.Name = "co2";
        Temperature.Name = "temperature";
        Calibrations.Name = "calibrations";
    }

protected:
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

    void Dump(const char (&data)[9], const TActorContext& context) {
        StringStream stream;
        for (size_t i = 0; i < 9; ++i) {
            if (i != 0) {
                stream << ':';
            }
            stream << String(int(data[i]), 16);
        }
        context.Send(this, Owner, new AW::TEventSensorMessage(*this, stream));
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        Serial.Begin();
        char request[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
        char response[9] = {};
        while (Serial.AvailableForRead()) {
            Serial.Read(response, 1);
        }
        Serial.Write(request, 9);
        int sz = Serial.Read(response, 9);
        if (sz == 9) {
            if (Env::Diagnostics) {
                Dump(response, context);
            }
            char request[9] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86}; // disable ABC
            Serial.Write(request, 9);
        } else {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "not found"));
            }
        }
        context.Send(this, this, new AW::TEventReceive());
        Calibrations = 0;
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());

        char request[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
        char response[9] = {};

        while (Serial.AvailableForRead()) {
            Serial.Read(response, 1);
        }
        Serial.Write(request, 9);
        // TODO: sleep?
        int sz = Serial.Read(response, 9);
        if (sz == 9 && (response[6]*256 + response[7]) != 15000 && response[4] != 0) {
            CO2 = float((uint16_t(response[2]) << 8) + response[3]);
            Temperature = float(int16_t(response[4]) - 40);

            if (Env::SensorsSendValues) {
                context.Send(this, Owner, new AW::TEventSensorData(*this, CO2));
                context.Send(this, Owner, new AW::TEventSensorData(*this, Temperature));
            }

            if (CO2.Value.GetValue() > 400) {
                LastCO2Seen = context.Now;
            }

            if (context.Now - LastCO2Seen > TTime::Minutes(20)) {
                char request[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78}; // zero point calibration
                Serial.Write(request, 9);
                Calibrations.SetValue(Calibrations.Value.GetValue() + 1);
                LastCO2Seen = context.Now;
                if (Env::SensorsSendValues) {
                    context.Send(this, Owner, new AW::TEventSensorData(*this, Calibrations));
                }
            }

            Updated = context.Now;

            if (Env::Diagnostics) {
                Dump(response, context);
            }
        } else {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "received BAD response"));
            }
        }
    }
};

}