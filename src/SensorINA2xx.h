#pragma once

#include "aw.h"

namespace AW {

template <typename Env = TDefaultEnvironment>
class TSensorINA2xx : public TActor, public TSensorSource {
    constexpr static bool UseChipCalculations = false;

    struct ERegisters {
        static constexpr uint8_t INA2xx_REG_CONFIG = 0x00;
        static constexpr uint8_t INA2xx_REG_SHUNTVOLTAGE = 0x01;
        static constexpr uint8_t INA2xx_REG_BUSVOLTAGE = 0x02;
        static constexpr uint8_t INA2xx_REG_POWER = 0x03;
        static constexpr uint8_t INA2xx_REG_CURRENT = 0x04;
        static constexpr uint8_t INA2xx_REG_CALIBRATION = 0x05;
        static constexpr uint8_t INA2xx_REG_MANUFACTURER_ID = 0xFE;
        static constexpr uint8_t INA2xx_REG_DIE_ID = 0xFF;
    };

    enum EFlags : uint16_t {
        /*---------------------------------------------------------------------*/
        INA219_CONFIG_RESET = 0x8000,  // Reset Bit

        INA219_CONFIG_BVOLTAGERANGE_MASK = 0x2000,  // Bus Voltage Range Mask
        INA219_CONFIG_BVOLTAGERANGE_16V = 0x0000,  // 0-16V Range
        INA219_CONFIG_BVOLTAGERANGE_32V = 0x2000,  // 0-32V Range

        INA219_CONFIG_GAIN_MASK = 0x1800,  // Gain Mask
        INA219_CONFIG_GAIN_1_40MV = 0x0000,  // Gain 1, 40mV Range
        INA219_CONFIG_GAIN_2_80MV = 0x0800,  // Gain 2, 80mV Range
        INA219_CONFIG_GAIN_4_160MV = 0x1000,  // Gain 4, 160mV Range
        INA219_CONFIG_GAIN_8_320MV = 0x1800,  // Gain 8, 320mV Range

        INA219_CONFIG_BADCRES_MASK = 0x0780,  // Bus ADC Resolution Mask
        INA219_CONFIG_BADCRES_9BIT = 0x0080,  // 9-bit bus res = 0..511
        INA219_CONFIG_BADCRES_10BIT = 0x0100,  // 10-bit bus res = 0..1023
        INA219_CONFIG_BADCRES_11BIT = 0x0200,  // 11-bit bus res = 0..2047
        INA219_CONFIG_BADCRES_12BIT = 0x0400,  // 12-bit bus res = 0..4097

        INA219_CONFIG_SADCRES_MASK = 0x0078,  // Shunt ADC Resolution and Averaging Mask
        INA219_CONFIG_SADCRES_9BIT_1S_84US = 0x0000,  // 1 x 9-bit shunt sample
        INA219_CONFIG_SADCRES_10BIT_1S_148US = 0x0008,  // 1 x 10-bit shunt sample
        INA219_CONFIG_SADCRES_11BIT_1S_276US = 0x0010,  // 1 x 11-bit shunt sample
        INA219_CONFIG_SADCRES_12BIT_1S_532US = 0x0018,  // 1 x 12-bit shunt sample
        INA219_CONFIG_SADCRES_12BIT_2S_1060US = 0x0048,	 // 2 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_4S_2130US = 0x0050,  // 4 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_8S_4260US = 0x0058,  // 8 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_16S_8510US = 0x0060,  // 16 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_32S_17MS = 0x0068,  // 32 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_64S_34MS = 0x0070,  // 64 x 12-bit shunt samples averaged together
        INA219_CONFIG_SADCRES_12BIT_128S_69MS = 0x0078,  // 128 x 12-bit shunt samples averaged together

        INA219_CONFIG_MODE_MASK = 0x0007,  // Operating Mode Mask
        INA219_CONFIG_MODE_POWERDOWN = 0x0000,
        INA219_CONFIG_MODE_SVOLT_TRIGGERED = 0x0001,
        INA219_CONFIG_MODE_BVOLT_TRIGGERED = 0x0002,
        INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED = 0x0003,
        INA219_CONFIG_MODE_ADCOFF = 0x0004,
        INA219_CONFIG_MODE_SVOLT_CONTINUOUS = 0x0005,
        INA219_CONFIG_MODE_BVOLT_CONTINUOUS = 0x0006,
        INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS = 0x0007,
        /*=========================================================================*/

        INA226_CONFIG_RESET = 0x8000,  // Reset Bit

        INA226_CONFIG_D14 = 0x4000,

        INA226_CONFIG_AVG_MASK = 0x0E00, // Averaging Mask
        INA226_CONFIG_AVG_1 = 0x0000, // 1 average
        INA226_CONFIG_AVG_4 = 0x0200, // 4 average
        INA226_CONFIG_AVG_16 = 0x0400, // 16 average
        INA226_CONFIG_AVG_64 = 0x0600, // 64 average
        INA226_CONFIG_AVG_128 = 0x0800, // 128 average
        INA226_CONFIG_AVG_256 = 0x0A00, // 256 average
        INA226_CONFIG_AVG_512 = 0x0C00, // 512 average
        INA226_CONFIG_AVG_1024 = 0x0E00, // 1024 average

        INA226_CONFIG_VBUSCT_MASK = 0x01C0, // Bus Voltage Conversion Time
        INA226_CONFIG_VBUSCT_140US = 0x0000,
        INA226_CONFIG_VBUSCT_204US = 0x0040,
        INA226_CONFIG_VBUSCT_332US = 0x0080,
        INA226_CONFIG_VBUSCT_588US = 0x00C0,
        INA226_CONFIG_VBUSCT_1100US = 0x0100,
        INA226_CONFIG_VBUSCT_2116US = 0x0140,
        INA226_CONFIG_VBUSCT_4156US = 0x0180,
        INA226_CONFIG_VBUSCT_8244US = 0x01C0,

        INA226_CONFIG_VSHCT_MASK = 0x0038, // Shunt Voltage Conversion Time
        INA226_CONFIG_VSHCT_140US = 0x0000,
        INA226_CONFIG_VSHCT_204US = 0x0008,
        INA226_CONFIG_VSHCT_332US = 0x0010,
        INA226_CONFIG_VSHCT_588US = 0x0018,
        INA226_CONFIG_VSHCT_1100US = 0x0020,
        INA226_CONFIG_VSHCT_2116US = 0x0028,
        INA226_CONFIG_VSHCT_4156US = 0x0030,
        INA226_CONFIG_VSHCT_8244US = 0x0038,

        INA226_CONFIG_MODE_MASK = 0x0007,  // Operating Mode Mask
        INA226_CONFIG_MODE_POWERDOWN = 0x0000,
        INA226_CONFIG_MODE_SVOLT_TRIGGERED = 0x0001,
        INA226_CONFIG_MODE_BVOLT_TRIGGERED = 0x0002,
        INA226_CONFIG_MODE_SANDBVOLT_TRIGGERED = 0x0003,
        INA226_CONFIG_MODE_ADCOFF = 0x0004,
        INA226_CONFIG_MODE_SVOLT_CONTINUOUS = 0x0005,
        INA226_CONFIG_MODE_BVOLT_CONTINUOUS = 0x0006,
        INA226_CONFIG_MODE_SANDBVOLT_CONTINUOUS = 0x0007,
    };

    struct TConfigRegister219 {
        uint16_t Mode : 3;
        uint16_t SADC : 4;
        uint16_t BADC : 4;
        uint16_t PG : 2;
        uint16_t BRNG : 1;
        uint16_t RSRVD : 1;
        uint16_t RESET : 1;
    };

    struct TBusVoltageRegister219 {
        int16_t OVF : 1;
        int16_t CNVR : 1;
        int16_t RSRVD : 1;
        int16_t Value : 13;
    };

    enum class EStage {
        Shot,
        Data,
    };

    EStage Stage = EStage::Shot;

    static constexpr TTime GetShotPeriod() { return TTime::MilliSeconds(69); /* 69ms */ }

public:
    uint8_t Address;
    uint16_t ChipId = 0;
    uint16_t ConfigValue;
    TActor* Owner;
    TAveragedSensorValue<fixed3_t, 60000 / Env::SensorsPeriod.MilliSeconds()> Voltage;
    TAveragedSensorValue<fixed3_t, 60000 / Env::SensorsPeriod.MilliSeconds()> Current;

    TSensorINA2xx(uint8_t address, TActor* owner, String name = "INA2xx")
        : Address(address)
        , Owner(owner)
    {
        Name = name;
        Voltage.Name = "voltage";
        Current.Name = "current";
    }

    static StringBuf GetSensorType(uint8_t address) {
        uint16_t manufacturer_id = 0;
        if (Env::Wire::ReadValue(address, ERegisters::INA2xx_REG_MANUFACTURER_ID, manufacturer_id)) {
            if (manufacturer_id == 0x5449) { // 'TI' Texas Instruments
                uint16_t die_id = 0;
                if (Env::Wire::ReadValue(address, ERegisters::INA2xx_REG_DIE_ID, die_id)) {
                    if (die_id == 0x2260) { // INA-226
                        return "ina226";
                    }
                }
            }
        }
        uint16_t config_value = 0; // INA2xx_REG_CONFIG
        if (Env::Wire::ReadValue(address, ERegisters::INA2xx_REG_CONFIG, config_value)) {
            return "ina219";
        }
        return StringBuf();
    }

protected:
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventReceive::EventID:
            switch (ChipId) {
                case 0x2190:
                    return OnReceive219(static_cast<TEventReceive*>(event.Release()), context);
                case 0x2260:
                    return OnReceive226(static_cast<TEventReceive*>(event.Release()), context);
            }
        default:
            break;
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
        if (!Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_DIE_ID, ChipId)) {
            ChipId = 0x2190;
        }

        switch (ChipId) {
            case 0xb40a: // ? have this value in my INA-219
            case 0xba12: // ? have this value in my INA-219
                ChipId = 0x2190;
                break;
        }

        switch (ChipId) {
            case 0x2190:
                ConfigValue =
                EFlags::INA219_CONFIG_BVOLTAGERANGE_16V |
                EFlags::INA219_CONFIG_GAIN_1_40MV |
                EFlags::INA219_CONFIG_BADCRES_12BIT |
                EFlags::INA219_CONFIG_SADCRES_12BIT_128S_69MS |
                //EFlags::INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED;
                EFlags::INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
                break;
            case 0x2260:
                ConfigValue =
                EFlags::INA226_CONFIG_D14 |
                EFlags::INA226_CONFIG_AVG_1 |
                EFlags::INA226_CONFIG_VBUSCT_204US |
                EFlags::INA226_CONFIG_VSHCT_204US |
                EFlags::INA226_CONFIG_MODE_SANDBVOLT_TRIGGERED;
                break;
        }

        if (Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, ConfigValue)) {
            if (UseChipCalculations) {
                static constexpr uint16_t CalibrationValue = 32768;
                Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CALIBRATION, CalibrationValue);
            }
            context.Send(this, this, new AW::TEventReceive());
            if (Env::Diagnostics) {
                switch (ChipId) {
                case 0x2190:
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "INA219 on " << String(Address, 16)));
                    break;
                case 0x2260:
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "INA226 on " << String(Address, 16)));
                    break;
                default:
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "INA 0x" << String(ChipId, 16) << " on " << String(Address, 16)));
                    break;
                }
            }
        } else {
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "NO INA2xx found"));
            }
        }
    }

    void OnReceive219(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        if (Stage == EStage::Shot) {
            Stage = EStage::Data;
            event->NotBefore = context.Now + GetShotPeriod();
            context.Resend(this, event.Release());
            Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, ConfigValue);
            return;
        }
        Stage = EStage::Shot;
        event->NotBefore = context.Now + Env::SensorsPeriod - GetShotPeriod();
        context.Resend(this, event.Release());

        uint16_t config_value = 0; // INA219_REG_CONFIG
        TConfigRegister219& config(*reinterpret_cast<TConfigRegister219*>(&config_value));

        uint16_t shunt_voltage = 0;
        uint16_t bus_voltage_value = 0;
        TBusVoltageRegister219& bus_voltage(*reinterpret_cast<TBusVoltageRegister219*>(&bus_voltage_value));

        Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_CONFIG, config_value);

        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "config " << String(config_value, 16)));
        }

        /*if (config_value != ConfigValue) {
            config_value = ConfigValue;
            Env::Wire::WriteValue(Address, ERegisters::INA219_REG_CONFIG, ConfigValue);
            if (Env::Diagnostics) {
                context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "replaced config to " << String(config_value, 16)));
            }
        }*/

        static constexpr float RSHUNT = 0.1; // ohms
        static constexpr float shuntLSB = 0.010; // mV
        static constexpr float voltageLSB = 0.004; // V

        Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_SHUNTVOLTAGE, shunt_voltage);
        Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_BUSVOLTAGE, bus_voltage_value);

        float busValue = bus_voltage.Value * voltageLSB;
        float shuntValue = ((int32_t)(int16_t)shunt_voltage << config.PG) * shuntLSB;

        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "bus " << String(bus_voltage_value, 16) << " (" << busValue << ")"));
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "shunt " << String(shunt_voltage, 16) << " (" << shuntValue << ")"));
        }

        if (shunt_voltage == 0xffff || bus_voltage.OVF) { // not connected or overflowed
            Voltage.Clear();
            Current.Clear();
            if (Env::SensorsCalibration) {
                if (bus_voltage.OVF) {
                    if (config.PG < 3) {
                        ++config.PG;
                        Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, config_value);
                        return;
                    }
                }
            }
        } else {
            if (bus_voltage.Value) {
                Voltage = busValue;
            } else {
                Voltage.Clear();
            }

            if (Env::SensorsCalibration) {
                float shuntValue = ((int32_t)(int16_t)shunt_voltage << config.PG) * shuntLSB;
                if (config.PG > 0) {
                    float maxShuntValue = ((int32_t)(int16_t)0x7fff << config.PG) * shuntLSB;
                    if (shuntValue < 0.50 * maxShuntValue) {
                        --config.PG;
                        Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, config_value);
                    }
                }
            }

            float currentValue;

            if (UseChipCalculations) {
                uint16_t calibration_value = 0;
                int16_t current_value = 0;
                int16_t power_value = 0;

                Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_CALIBRATION, calibration_value);
                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "calibration " << String(calibration_value, 16)));
                }

                //float currentLSB = 0.04096 / (calibration_value * RSHUNT); // A
                float currentLSB = 40.96 / (calibration_value * RSHUNT); // mA
                float powerLSB = currentLSB * 20; // mW

                Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_CURRENT, current_value);
                Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_POWER, power_value);

                currentValue = current_value * currentLSB;
                Current = currentValue;
                float powerValue = power_value * powerLSB;
                if (currentValue < 0) {
                    powerValue = -powerValue;
                }

                if (Env::Diagnostics) {
                    context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "current " << String(current_value, 16) << " (" << Current.Value << ")"));
                }

                if (Env::SensorsCalibration) {
                    float maxCurrentValue = 0x7fff * currentLSB;
                    static constexpr uint16_t calibration_value_step = 1024;
                    if (abs(currentValue) < 0.40 * maxCurrentValue) {
                        if (calibration_value <= (0xffff - calibration_value_step * 2)) {
                            calibration_value += calibration_value_step;
                            Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CALIBRATION, calibration_value);
                        }
                    } else if (abs(currentValue) > 0.90 * maxCurrentValue) {
                        if (calibration_value >= calibration_value_step * 2) {
                            calibration_value -= calibration_value_step;
                            Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CALIBRATION, calibration_value);
                        }
                    }
                }
            } else {
                currentValue = shuntValue / RSHUNT;
                Current = currentValue;
                //Serial.print("current "); Serial.println(currentValue); //Serial.flush();
            }
        }

        uint16_t configValue = EFlags::INA219_CONFIG_MODE_POWERDOWN;

        Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, configValue);

        Updated = context.Now;

        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(*this, Voltage));
            context.Send(this, Owner, new AW::TEventSensorData(*this, Current));
        }
    }

    void OnReceive226(AW::TUniquePtr<AW::TEventReceive> event, const AW::TActorContext& context) {
        if (Stage == EStage::Shot) {
            Stage = EStage::Data;
            event->NotBefore = context.Now + GetShotPeriod();
            context.Resend(this, event.Release());
            Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, ConfigValue);
            return;
        }
        ulong start;
        if (Env::Diagnostics) {
            start = micros();
        }
        Stage = EStage::Shot;
        event->NotBefore = context.Now + Env::SensorsPeriod - GetShotPeriod();
        context.Resend(this, event.Release());

        int16_t shunt_voltage = 0;
        uint16_t bus_voltage = 0;

        static constexpr float RSHUNT = 0.02; // ohms
        static constexpr float shuntLSB = 0.0025; // mV
        static constexpr float voltageLSB = 0.00125; // V

        Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_SHUNTVOLTAGE, shunt_voltage);
        Env::Wire::ReadValue(Address, ERegisters::INA2xx_REG_BUSVOLTAGE, bus_voltage);

        float busValue = float(bus_voltage) * voltageLSB;
        float shuntValue = float(shunt_voltage) * shuntLSB;

        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "bus " << String(bus_voltage, 16) << " (" << busValue << ")"));
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "shunt " << String(shunt_voltage, 16) << " (" << shuntValue << ")"));
        }

        if (shunt_voltage == 0xffff) { // not connected or overflowed
            Voltage.Clear();
            Current.Clear();
        } else {
            if (busValue) {
                Voltage = busValue;
            } else {
                Voltage.Clear();
            }

            float currentValue;

            currentValue = shuntValue / RSHUNT;
            Current = currentValue;
        }

        uint16_t configValue = EFlags::INA226_CONFIG_MODE_POWERDOWN;

        Env::Wire::WriteValue(Address, ERegisters::INA2xx_REG_CONFIG, configValue);

        Updated = context.Now;

        if (Env::SensorsSendValues) {
            context.Send(this, Owner, new AW::TEventSensorData(*this, Voltage));
            context.Send(this, Owner, new AW::TEventSensorData(*this, Current));
        }
        if (Env::Diagnostics) {
            context.Send(this, Owner, new AW::TEventSensorMessage(*this, StringStream() << "receive elapsed " << (micros() - start) << "us"));
        }
    }
};

}