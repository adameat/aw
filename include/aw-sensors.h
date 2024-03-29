#pragma once

#include "aw.h"
#include "aw-serial.h"
#include "aw-bluetooth.h"
#include "aw-average.h"
#include "aw-string-buf.h"
#include "aw-time.h"

extern Uart ConsoleSerial;

namespace AW {

template <typename TBaseType>
class TOptionalValueBase;

template <>
class TOptionalValueBase<float> {
public:
    using TUnderlyingType = float;

    TOptionalValueBase()
        : Value(NAN)
    {}

    TOptionalValueBase(float value)
        : Value(value)
    {}

    void SetValue(float value) {
        Value = value;
    }

    float GetValue() const {
        return Value;
    }

    void Clear() {
        Value = NAN;
    }

    bool IsValid() const {
        return !isnan(Value);
    }

    void operator =(float value) {
        SetValue(value);
    }

    operator float() const {
        return GetValue();
    }

protected:
    float Value;
};

template <>
class TOptionalValueBase<fixed3_t> {
public:
    using TUnderlyingType = fixed3_t;

    TOptionalValueBase()
    {
        Value.setnan();
    }

    TOptionalValueBase(fixed3_t value)
        : Value(value)
    {}

    void SetValue(fixed3_t value) {
        Value = value;
    }

    fixed3_t GetValue() const {
        return Value;
    }

    void Clear() {
        Value.setnan();
    }

    bool IsValid() const {
        return !Value.isnan();
    }

    void operator =(fixed3_t value) {
        SetValue(value);
    }

    operator fixed3_t() const {
        return GetValue();
    }

protected:
    fixed3_t Value;
};

template <>
class TOptionalValueBase<unsigned long> {
public:
    using TUnderlyingType = unsigned long;

    TOptionalValueBase()
    {
        Value = ~0UL;
    }

    TOptionalValueBase(unsigned long value)
        : Value(value)
    {}

    void SetValue(unsigned long value) {
        Value = value;
    }

    unsigned long GetValue() const {
        return Value;
    }

    void Clear() {
        Value = ~0UL;
    }

    bool IsValid() const {
        return Value != ~0UL;
    }

    void operator =(unsigned long value) {
        SetValue(value);
    }

    operator unsigned long() const {
        return GetValue();
    }

protected:
    unsigned long Value;
};

using TOptionalValue = TOptionalValueBase<fixed3_t>;

template <typename ValueType>
struct TSensorValue {
    StringBuf Name;
    ValueType Value;

    TSensorValue() = default;

    TSensorValue(StringBuf name, ValueType value = ValueType())
        : Name(name)
        , Value(value)
    {}

    bool operator ==(const TSensorValue<ValueType>& other) const {
        return this == &other;
    }

    void operator =(typename ValueType::TUnderlyingType value) {
        Value = value;
    }

    void SetValue(typename ValueType::TUnderlyingType value) {
        Value = value;
    }

    typename ValueType::TUnderlyingType GetValue() const {
        return Value;
    }

    void Clear() {
        Value.Clear();
    }
};

using TSensorValueFixed3 = TSensorValue<TOptionalValueBase<fixed3_t>>;
using TSensorValueFloat = TSensorValue<TOptionalValueBase<float>>;
using TSensorValueULong = TSensorValue<TOptionalValueBase<unsigned long>>;

template <typename ValueType, int WindowsSize = 60>
struct TAveragedSensorValue : TSensorValue<TOptionalValueBase<ValueType>> {
public:
    using TBase = TSensorValue<TOptionalValueBase<ValueType>>;
    TAverage<ValueType, WindowsSize> Average;

    void operator =(ValueType value) {
        Average.AddValue(value);
        TBase::SetValue(Average.GetValue());
    }

    void Clear() {
        Average.Clear();
        TBase::Clear();
    }

    void SetValue(ValueType value) {
        Average.SetValue(value);
        TBase::SetValue(value);
    }
};

struct TSensorSource {
    String Name;
    TTime Updated;

    bool operator ==(const TSensorSource& other) const {
        return this == &other;
    }
};

// template <size_t Count> struct TSensor : TSensorSource {
//     TSensorValue Values[Count];
// };

struct TEventSensorData : TBasicEvent<TEventSensorData> {
    constexpr static TEventID EventID = TEventID::EventSensorData;
    const TSensorSource& Source;
    StringBuf Name;
    String Value;

    TEventSensorData(const TSensorSource& source, StringBuf name, String value)
        : Source(source)
        , Name(Move(name))
        , Value(Move(value))
    {}

    template <typename TSensorValue>
    TEventSensorData(const TSensorSource& source, const TSensorValue& sensorValue)
        : TEventSensorData(source, sensorValue.Name, sensorValue.Value.GetValue())
    {}
};

struct TEventSensorMessage : TBasicEvent<TEventSensorMessage> {
    constexpr static TEventID EventID = TEventID::EventSensorMessage;
    const TSensorSource& Source;
    String Message;

    TEventSensorMessage(const TSensorSource& source, const String& message)
        : Source(source)
        , Message(message) {}
};

template <typename Env = TDefaultEnvironment, bool HaveConsole = Env::HaveConsole>
class TConsoleActor;

template <typename Env>
class TConsoleActor<Env, false> {
public:
    TDummyActor Console;

    TConsoleActor(TActor*) {}
};

template <typename Env>
class TConsoleActor<Env, true> {
public:
    typename Env::ConsoleActor Console;

    TConsoleActor(TActor* owner)
        : Console(owner)
    {}
};

template <typename Env = TDefaultEnvironment>
class TSensorActor : public TActor, public TConsoleActor<Env> {
public:
    using TConsoleActor<Env>::Console;
    TSyncSerialActor<THardwareSerial<Serial1, Env::BluetoothBaudRate>> Channel;
    TDigitalPin<Env::PIN_POWER_BLUETOOTH> PowerBluetooth;
    TDigitalPin<Env::PIN_POWER_I2C> PowerI2C;
    TDigitalPin<Env::PIN_LED_SLEEP> SleepLED;
    TLed Led;
    bool Feed = false;
    static constexpr TTime DefaultPeriod = Env::DefaultPeriod;
    TTime Period = DefaultPeriod;
    TEventReceive* EventReceive;
    TTime LastReportTime;
    TTime ConnectAliveTime;
    TSensorSource TimeSource;
    TSensorValueULong TimeTotal;
    TSensorValueULong TimeBusy;
    TSensorValueULong TimeSleep;
    TActorLib* ActorLib;

    TSensorActor()
        : TConsoleActor<Env>(this)
        , Channel(this)
        , ActorLib(nullptr)
    {
        TimeSource.Name = "time";
        TimeTotal.Name = "total";
        TimeBusy.Name = "busy";
        TimeSleep.Name = "sleep";
    }

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return SensorBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventData::EventID:
            return SensorData(static_cast<TEventData*>(event.Release()), context);
        case TEventReceive::EventID:
            return SensorReceive(static_cast<TEventReceive*>(event.Release()), context);
        case TEventSensorData::EventID:
            return SensorSensorData(static_cast<TEventSensorData*>(event.Release()), context);
        case TEventSensorMessage::EventID:
            return SensorSensorMessage(static_cast<TEventSensorMessage*>(event.Release()), context);
        default:
            break;
        }
        if (Env::SupportsSleep) {
            switch (event->EventID) {
            case TEventSleep::EventID:
                return SensorSleep(static_cast<TEventSleep*>(event.Release()), context);
            case TEventWakeUp::EventID:
                return SensorWakeUp(static_cast<TEventWakeUp*>(event.Release()), context);
            default:
                break;
            }
        }
    }

    virtual void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext&) {}
    virtual void OnSensorData(TUniquePtr<TEventSensorData>, const TActorContext&) {}
    virtual void OnReceive(const TActorContext&) {}

    virtual bool OnCommand(const AW::String&, const AW::TActorContext&) {
        return false;
    }

    virtual void OnSendSensors(const TActorContext& context) {
        TimeSource.Updated = context.Now;
        TimeTotal.Value.SetValue(context.Now.MilliSeconds());
        TimeBusy.Value.SetValue(context.ActorLib.BusyTime.MilliSeconds());
        if (Env::SupportsSleep) {
            TimeSleep.Value.SetValue(context.ActorLib.SleepTime.MilliSeconds());
        }
        SendSensorValues(context, TimeSource, TimeTotal, TimeBusy, TimeSleep);
    }

    void SendSensors(const TActorContext& context) {
        OnSendSensors(context);
        LastReportTime = context.Now;
    }

    void SensorSleep(TUniquePtr<TEventSleep> event, const TActorContext& context) {
        if (Env::SupportsSleep) {
            if (Env::Diagnostics && Env::HaveConsole) {
                context.Send(this, &Console, new TEventData("sleep"));
            }
            if (Env::HaveConsole) {
                context.Send(this, &Console, new TEventSleep());
            }
            context.Send(this, &Channel, new TEventSleep());
            context.ActorLib.Sleep();
            PowerBluetooth = false;
            SleepLED = true;
            //PowerI2C = false;
        }
    }

    void SensorWakeUp(TUniquePtr<TEventWakeUp> event, const TActorContext& context) {
        if (Env::SupportsSleep) {
            SleepLED = false;
            PowerBluetooth = true;
            context.ActorLib.WakeUp();
            if (Env::HaveConsole) {
                context.Send(this, &Console, new TEventWakeUp());
            }
            context.Send(this, &Channel, new TEventWakeUp());
            if (Env::Diagnostics && Env::HaveConsole) {
                context.Send(this, &Console, new TEventData("wakeup"));
            }
            /*PowerI2C = true;
            delay(10);
            Env::Wire::Begin();*/
        }
    }

    void SensorBootstrap(TUniquePtr<TEventBootstrap> event, const TActorContext& context) {
        ActorLib = &context.ActorLib;
        Led = true;

        PowerI2C = true;
        PowerBluetooth = true;
        delay(10);
        Env::Wire::Begin();

        if (Env::HaveConsole) {
            context.ActorLib.Register(&Console);
        }
        context.ActorLib.Register(&Channel);
        //context.ActorLib.Register(&Bluetooth);
        if (Env::HaveConsole) {
            context.Send(this, &Console, new TEventData(StringStream() << "\nhi"));
        } else {
            //context.Send(this, &Channel, new TEventData("hi"));
        }
        context.Send(this, this, EventReceive = new TEventReceive());
        OnBootstrap(Move(event), context);
        Led = false;
    }

    void SensorData(TUniquePtr<TEventData> event, const TActorContext& context) {
        Led = true;
        ConnectAliveTime = context.Now;
        StringBuf data(event->Data);
        while (!data.empty() && (data[0] <= 32 || data[0] > 127)) {
            data = data.substr(1);
        }
        if (OnCommand(data, context)) {
            // do nothing
        } else if (data == "OK") {
            // do nothing
        } else if (data == "PING") {
            context.Send(this, event->Sender, new TEventData("PONG"));
        } else if (data.ends_with("CONNECTED") || data.starts_with("+")) {
            Feed = false;
            Period = TTime::Seconds(30);
            Channel.PurgeEvents(TEventData::EventID);
        } else if (data.starts_with("FEED")) {
            Feed = true;
            if (data.size() > 4) {
                Period = TTime::Seconds(event->Data.substr(5));
            } else {
                Period = DefaultPeriod;
            }
            EventReceive->NotBefore = context.Now/* + Period*/;
        } else if (Env::SupportsSleep && data.starts_with("SLEEP")) {
            if (data.size() > 5) {
                TTime period = TTime::Seconds(data.substr(6));
                context.Send(this, this, new TEventSleep());
                context.Send(this, this, new TEventWakeUp(context.Now + period));
                Feed = false;
                Period = DefaultPeriod;
            }
        } else if (data == "READ") {
            SendSensors(context);
            context.Send(this, &Channel, new TEventData("DONE"));
        } else if (data == "STOP") {
            Feed = false;
            Period = DefaultPeriod;
        } else if (data.starts_with("RESET")) {
            StringBuf command(data);
            command.NextToken(' ');
            StringBuf reason = "CMD";
            if (!command.empty()) {
                reason = command;
            }
            Reset(reason);
        } else {
            context.Send(this, event->Sender, new TEventData(StringStream() << "WRONG " << data));
        }
        if (Env::HaveConsole && event->Sender != &Console) {
            context.Send(this, &Console, event.Release());
        }
        Led = false;
    }

    template <typename ValueType>
    void SendSensorValue(const TActorContext& context, StringBuf sourceName, StringBuf valueName, ValueType value) {
        StringStream stream;
        stream << "DATA " << sourceName << '.' << valueName << ' ' << value << " OK";
        if (Env::UseSum) {
            auto size = stream.size();
            stream << ' ' << size;
        }
        if (Env::UseCRC16) {
            String crc16(stream.str().crc16(), 16);
            stream << ' ';
            for (auto i = crc16.size(); i < 4; ++i) {
                stream << '0';
            }
            stream << crc16;
        }
        context.Send(this, &Channel, new TEventData(stream));
    }

    template <typename ValueType>
    void SendSensorValue(const TSensorSource& source, const ValueType& value, const TActorContext& context) {
        if (source.Updated >= LastReportTime && value.Value.IsValid()) {
            SendSensorValue(context, source.Name, value.Name, value.Value);
        }
    }

    /*template <size_t Count>
    void SendSensors(const TSensor<Count>& sensor, const TActorContext& context) {
        for (const TSensorValue& value : sensor.Values) {
            SendSensorValue(sensor, value, context);
        }
    }*/

    void SendSensorValues(const TActorContext&, const TSensorSource&) {}

    template <typename SensorValue, typename... SensorValues>
    void SendSensorValues(const TActorContext& context, const TSensorSource& sensor, const SensorValue& value, const SensorValues&... values) {
        SendSensorValue(sensor, value, context);
        SendSensorValues(context, sensor, values...);
    }

    /*template <typename SensorType>
    void SendSensors(SensorType& sensor, const TActorContext& context) {
        for (TSensorValue SensorType::* value : sensor.Values) {
            SendSensorValue(sensor, sensor.*value, context);
        }
    }*/

    void SensorSensorData(TUniquePtr<TEventSensorData> event, const TActorContext& context) {
        //if (Feed) {
        //    SendSensorValue(event->Source, event->Value, context);
        //}
        if (Env::HaveConsole && Env::DumpSensorData) {
            context.Send(this, &Console, new TEventData(StringStream() << "DATA " << event->Source.Name << '.' << event->Name << ' ' << event->Value));
        }
        OnSensorData(Move(event), context);
    }

    void SensorSensorMessage(TUniquePtr<TEventSensorMessage> event, const TActorContext& context) {
        if (Env::HaveConsole) {
            context.Send(this, &Console, new TEventData(event->Source.Name));
            context.Send(this, &Console, new TEventData(event->Message));
        }
    }

    void SensorReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        Led = true;
        if (Env::HaveConsole && !Feed) {
            context.Send(this, &Console, new TEventData(StringStream() << "RCV " << context.Now.Seconds() << " " << ConnectAliveTime.Seconds() << " " << LastReportTime.Seconds()));
        }
        if (!context.ActorLib.Sleeping) {
            if (LastReportTime + TTime::Minutes(5) < context.Now) {
                Reset("MIN5-D");
            }
            if (TTime::Hours(24) < context.Now) {
                Reset("HOUR24");
            }
            if (ConnectAliveTime + TTime::Minutes(3) < context.Now) {
                Reset("MIN3-C");
            }
            if (Feed) {
                //ConnectAliveTime = context.Now;
                SendSensors(context);
            }/* else {
                if (!Connected) {
                    context.Send(this, &Channel, new TEventData("AT"));
                }
            }*/
        }
        OnReceive(context);
        event->NotBefore = context.Now + Period;
        context.Resend(this, event.Release());
        Led = false;
    }

    void Reset(StringBuf reason) {
        if (Env::HaveConsole) {
            ActorLib->SendSync(&Console, new TEventData(reason));
        }
        PowerI2C = false;
        PowerBluetooth = false;
        for (int i = 0; i < 15; ++i) {
            Led = true;
            delayMicroseconds(25000);
            Led = false;
            delayMicroseconds(50000);
        }
        AW::DefaultReset(reason);
    }

    template <typename SensorType>
    SensorType* DetectSensor(uint8_t address) {
        StringBuf type = SensorType::GetSensorType(address);
        if (!type.empty()) {
            String name = StringStream() << type << '-' << String(address,16);
            SensorType* sensor = new SensorType(address, this, name);
            if (Env::HaveConsole) {
                ActorLib->Send(&Console, new TEventData(StringStream() << "Found " << name));
            }
            ActorLib->Register(sensor);
            return sensor;
        } else {
            return nullptr;
        }
    }
};

}
