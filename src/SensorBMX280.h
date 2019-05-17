#pragma once

#include "aw.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorBMX280 : public TActor, public TSensorSource {
    enum EChips : uint8_t {
        Unknown = 0x00,
        BMP280 = 0x58,
        BME280 = 0x60
    };

    enum ERegisters : uint8_t {
        REGISTER_DIG_T1 = 0x88,
        REGISTER_DIG_T2 = 0x8A,
        REGISTER_DIG_T3 = 0x8C,

        REGISTER_DIG_P1 = 0x8E,
        REGISTER_DIG_P2 = 0x90,
        REGISTER_DIG_P3 = 0x92,
        REGISTER_DIG_P4 = 0x94,
        REGISTER_DIG_P5 = 0x96,
        REGISTER_DIG_P6 = 0x98,
        REGISTER_DIG_P7 = 0x9A,
        REGISTER_DIG_P8 = 0x9C,
        REGISTER_DIG_P9 = 0x9E,

        REGISTER_DIG_H1 = 0xA1,
        REGISTER_DIG_H2 = 0xE1,
        REGISTER_DIG_H3 = 0xE3,
        REGISTER_DIG_H4 = 0xE4,
        REGISTER_DIG_H5 = 0xE5,
        REGISTER_DIG_H6 = 0xE7,

        REGISTER_CHIPID = 0xD0,
        REGISTER_VERSION = 0xD1,
        REGISTER_SOFTRESET = 0xE0,

        REGISTER_CAL26 = 0xE1,  // R calibration stored in 0xE1-0xF0

        REGISTER_CONTROLHUMID = 0xF2,
        REGISTER_STATUS = 0XF3,
        REGISTER_CONTROL = 0xF4,
        REGISTER_CONFIG = 0xF5,
        REGISTER_PRESSUREDATA = 0xF7,
        REGISTER_TEMPDATA = 0xFA,
        REGISTER_HUMIDDATA = 0xFD,
    };

    struct CalibData {
        uint16_t dig_T1;
        int16_t  dig_T2;
        int16_t  dig_T3;

        uint16_t dig_P1;
        int16_t  dig_P2;
        int16_t  dig_P3;
        int16_t  dig_P4;
        int16_t  dig_P5;
        int16_t  dig_P6;
        int16_t  dig_P7;
        int16_t  dig_P8;
        int16_t  dig_P9;

        uint8_t  dig_H1;
        int16_t  dig_H2;
        uint8_t  dig_H3;
        int16_t  dig_H4;
        int16_t  dig_H5;
        int8_t   dig_H6;
    };

    enum sensor_sampling {
        SAMPLING_NONE = 0b000,
        SAMPLING_X1 = 0b001,
        SAMPLING_X2 = 0b010,
        SAMPLING_X4 = 0b011,
        SAMPLING_X8 = 0b100,
        SAMPLING_X16 = 0b101,
    };

    enum sensor_mode {
        MODE_SLEEP = 0b00,
        MODE_FORCED = 0b01,
        MODE_NORMAL = 0b11,
    };

    enum sensor_filter {
        FILTER_OFF = 0b000,
        FILTER_X2 = 0b001,
        FILTER_X4 = 0b010,
        FILTER_X8 = 0b011,
        FILTER_X16 = 0b100,
    };

    // standby durations in ms
    enum standby_duration {
        STANDBY_MS_0_5 = 0b000,
        STANDBY_MS_10 = 0b110,
        STANDBY_MS_20 = 0b111,
        STANDBY_MS_62_5 = 0b001,
        STANDBY_MS_125 = 0b010,
        STANDBY_MS_250 = 0b011,
        STANDBY_MS_500 = 0b100,
        STANDBY_MS_1000 = 0b101,
    };

    // The config register
    struct config {
        // inactive duration (standby time) in normal mode
        // 000 = 0.5 ms
        // 001 = 62.5 ms
        // 010 = 125 ms
        // 011 = 250 ms
        // 100 = 500 ms
        // 101 = 1000 ms
        // 110 = 10 ms
        // 111 = 20 ms
        uint8_t t_sb : 3;

        // filter settings
        // 000 = filter off
        // 001 = 2x filter
        // 010 = 4x filter
        // 011 = 8x filter
        // 100 and above = 16x filter
        uint8_t filter : 3;

        // unused - don't set
        uint8_t none : 1;
        uint8_t spi3w_en : 1;

        uint8_t get() {
            return (t_sb << 5) | (filter << 3) | spi3w_en;
        }
    };

    // The ctrl_meas register
    struct ctrl_meas {
        // temperature oversampling
        // 000 = skipped
        // 001 = x1
        // 010 = x2
        // 011 = x4
        // 100 = x8
        // 101 and above = x16
        uint8_t osrs_t : 3;

        // pressure oversampling
        // 000 = skipped
        // 001 = x1
        // 010 = x2
        // 011 = x4
        // 100 = x8
        // 101 and above = x16
        uint8_t osrs_p : 3;

        // device mode
        // 00       = sleep
        // 01 or 10 = forced
        // 11       = normal
        uint8_t mode : 2;

        uint8_t get() {
            return (osrs_t << 5) | (osrs_p << 3) | mode;
        }
    };

    // The ctrl_hum register
    struct ctrl_hum {
        // unused - don't set
        uint8_t none : 5;

        // pressure oversampling
        // 000 = skipped
        // 001 = x1
        // 010 = x2
        // 011 = x4
        // 100 = x8
        // 101 and above = x16
        uint8_t osrs_h : 3;

        uint8_t get() {
            return (osrs_h);
        }
    };

public:
    uint8_t Address = 0x77;
    EChips ChipID = EChips::Unknown;
    TActor* Owner;
    TSensorValue Temperature;
    TSensorValue Pressure;
    TSensorValue Humidity;
    //static constexpr TSensorValue TSensor::* Values[] = { &TSensor::Temperature, &TSensor::Pressure, &TSensor::Humidity };

    TSensorBMX280(uint8_t address, TActor* owner, StringBuf name = "bmx280")
        : Address(address)
        , Owner(owner)
    {
        Name = name;
        Temperature.Name = "temperature";
        Pressure.Name = "pressure";
        Humidity.Name = "humidity";
    }

protected:
    CalibData Calib;

    static constexpr sensor_sampling tempSampling = SAMPLING_X4;
    static constexpr sensor_sampling pressSampling = SAMPLING_X4;
    static constexpr sensor_sampling humSampling = SAMPLING_X4;
    static constexpr sensor_filter filter = FILTER_OFF;
    static constexpr standby_duration duration = STANDBY_MS_1000;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        case TEventSleep::EventID:
            return OnSleep(static_cast<TEventSleep*>(event.Release()), context);
        case TEventWakeUp::EventID:
            return OnWakeUp(static_cast<TEventWakeUp*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        Env::Wire::ReadValue(Address, ERegisters::REGISTER_CHIPID, ChipID);
        if (ChipID == EChips::BME280 || ChipID == EChips::BMP280) {
            /*
            // reset the device using soft-reset
            // this makes sure the IIR is off, etc.
            write8(BME280_REGISTER_SOFTRESET, 0xB6);

            // wait for chip to wake up.
            delay(300);

            // if chip is still reading calibration, delay
            while (isReadingCalibration())
                delay(100);
            */

            if (ChipID == EChips::BME280) {
                sensor_mode mode = MODE_NORMAL;

                config _configReg;
                ctrl_meas _measReg;
                ctrl_hum _humReg;

                _measReg.mode = mode;
                _measReg.osrs_t = tempSampling;
                _measReg.osrs_p = pressSampling;
                _humReg.osrs_h = humSampling;
                _configReg.filter = filter;
                _configReg.t_sb = duration;

                // you must make sure to also set REGISTER_CONTROL after setting the
                // CONTROLHUMID register, otherwise the values won't be applied (see DS 5.4.3)
                Env::Wire::WriteValue(Address, REGISTER_CONTROLHUMID, _humReg.get());
                Env::Wire::WriteValue(Address, REGISTER_CONFIG, _configReg.get());
                Env::Wire::WriteValue(Address, REGISTER_CONTROL, _measReg.get());
            }

            if (Env::Diagnostics) {
                StringStream stream;
                switch (ChipID) {
                case EChips::BME280:
                    stream << "BME280";
                    break;
                case EChips::BMP280:
                    stream << "BMP280";
                    break;
                default:
                    stream << (uint8_t)ChipID;
                    break;
                }
                stream << " on " << String(Address, 16);
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, stream));
            }
            context.Send(this, this, new AW::TEventReceive());
        } else {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "NO BMx280 found"));
            }
        }
    }

    void OnSleep(AW::TUniquePtr<AW::TEventSleep> event, const AW::TActorContext& context) {
        /*sensor_mode mode = MODE_SLEEP;
        ctrl_meas _measReg;
        _measReg.mode = mode;
        _measReg.osrs_t = tempSampling;
        _measReg.osrs_p = pressSampling;
        Env::Wire::WriteValue(Address, REGISTER_CONTROL, _measReg.get());*/
    }

    void OnWakeUp(AW::TUniquePtr<AW::TEventWakeUp> event, const AW::TActorContext& context) {
        /*sensor_mode mode = MODE_NORMAL;
        ctrl_meas _measReg;
        _measReg.mode = mode;
        _measReg.osrs_t = tempSampling;
        _measReg.osrs_p = pressSampling;
        Env::Wire::WriteValue(Address, REGISTER_CONTROL, _measReg.get());*/
    }

    void ReadCoefficients(CalibData& data) {
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_T1, data.dig_T1);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_T2, data.dig_T2);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_T3, data.dig_T3);

        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P1, data.dig_P1);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P2, data.dig_P2);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P3, data.dig_P3);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P4, data.dig_P4);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P5, data.dig_P5);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P6, data.dig_P6);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P7, data.dig_P7);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P8, data.dig_P8);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_P9, data.dig_P9);

        Env::Wire::ReadValue(Address, REGISTER_DIG_H1, data.dig_H1);
        Env::Wire::ReadValueLE(Address, REGISTER_DIG_H2, data.dig_H2);
        Env::Wire::ReadValue(Address, REGISTER_DIG_H3, data.dig_H3);

        uint8_t h[3];
        Env::Wire::ReadValue(Address, REGISTER_DIG_H4, h[0]);
        Env::Wire::ReadValue(Address, REGISTER_DIG_H5, h[1]);
        Env::Wire::ReadValue(Address, REGISTER_DIG_H5 + 1, h[2]);
        data.dig_H4 = (h[0] << 4) | (h[1] & 0xF);
        data.dig_H5 = (h[2] << 4) | (h[1] >> 4);

        Env::Wire::ReadValue(Address, REGISTER_DIG_H6, data.dig_H6);
    }

    void OnReceive(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        uint8_t status;
        if (Env::Wire::ReadValue(Address, ERegisters::REGISTER_STATUS, status)) {
            if ((status & 1) == 0) {
                Updated = context.Now;
                    
                int32_t t_fine = 0;
                uint24_t temp;
                if (Env::Wire::ReadValueLE(Address, ERegisters::REGISTER_TEMPDATA, temp) && temp != 0x800000) {
                    int32_t var1, var2;
                    int32_t adc_T = temp;
                    adc_T >>= 4;

                    var1 = ((((adc_T >> 3) - ((int32_t)Calib.dig_T1 << 1))) *
                        ((int32_t)Calib.dig_T2)) >> 11;

                    var2 = (((((adc_T >> 4) - ((int32_t)Calib.dig_T1)) *
                        ((adc_T >> 4) - ((int32_t)Calib.dig_T1))) >> 12) *
                        ((int32_t)Calib.dig_T3)) >> 14;

                    t_fine = var1 + var2;

                    float T = (t_fine * 5 + 128) >> 8;
                    Temperature.Value = T / 100;
                    if (Env::SensorsSendValues) {
                        context.Send(this, Owner, new AW::TEventSensorData(*this, Temperature));
                    }
                }
                uint24_t pressure;
                if (Env::Wire::ReadValueLE(Address, ERegisters::REGISTER_PRESSUREDATA, pressure) && pressure != 0x800000) {
                    int64_t var1, var2, p;
                    int32_t adc_P = pressure;
                    adc_P >>= 4;

                    var1 = ((int64_t)t_fine) - 128000;
                    var2 = var1 * var1 * (int64_t)Calib.dig_P6;
                    var2 = var2 + ((var1*(int64_t)Calib.dig_P5) << 17);
                    var2 = var2 + (((int64_t)Calib.dig_P4) << 35);
                    var1 = ((var1 * var1 * (int64_t)Calib.dig_P3) >> 8) +
                        ((var1 * (int64_t)Calib.dig_P2) << 12);
                    var1 = (((((int64_t)1) << 47) + var1))*((int64_t)Calib.dig_P1) >> 33;

                    if (var1 != 0) {
                        p = 1048576 - adc_P;
                        p = (((p << 31) - var2) * 3125) / var1;
                        var1 = (((int64_t)Calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
                        var2 = (((int64_t)Calib.dig_P8) * p) >> 19;

                        p = ((p + var1 + var2) >> 8) + (((int64_t)Calib.dig_P7) << 4);
                        float P = (float)p / 256;
                        Pressure.Value = P / 133.32239; // to mmHg
                        if (Env::SensorsSendValues) {
                            context.Send(this, Owner, new AW::TEventSensorData(*this, Pressure));
                        }
                    }
                }
                if (ChipID == EChips::BME280) {
                    uint16_t humidity;
                    if (Env::Wire::ReadValue(Address, ERegisters::REGISTER_HUMIDDATA, humidity) && humidity != 0x8000) {
                        int32_t adc_H = humidity;
                        int32_t v_x1_u32r;

                        v_x1_u32r = (t_fine - ((int32_t)76800));

                        v_x1_u32r = (((((adc_H << 14) - (((int32_t)Calib.dig_H4) << 20) -
                            (((int32_t)Calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                            (((((((v_x1_u32r * ((int32_t)Calib.dig_H6)) >> 10) *
                            (((v_x1_u32r * ((int32_t)Calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                                ((int32_t)2097152)) * ((int32_t)Calib.dig_H2) + 8192) >> 14));

                        v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                            ((int32_t)Calib.dig_H1)) >> 4));

                        v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
                        v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
                        float h = (v_x1_u32r >> 12);
                        Humidity.Value = h / 1024.0;
                        if (Env::SensorsSendValues) {
                            context.Send(this, Owner, new AW::TEventSensorData(*this, Humidity));
                        }
                    }
                }
            }
        }

        event->NotBefore = context.Now + Env::SensorsPeriod;
        context.Resend(this, event.Release());
    }
};

}