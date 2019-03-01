#pragma once

#include "aw.h"
#include "aw-serial.h"
#include "aw-bluetooth.h"
#include "aw-average.h"
#include "aw-string-buf.h"
#include "aw-time.h"

namespace AW {

class TOptionalValue {
public:
    TOptionalValue()
        : Value(NAN)
    {}

    void SetValue(double value) {
        Value = value;
    }

    double GetValue() const {
        return Value;
    }

    void Clear() {
        Value = NAN;
    }

    bool IsValid() const {
        return !isnan(Value);
    }

    void operator =(double value) {
        SetValue(value);
    }

    operator double() const {
        return GetValue();
    }

protected:
    double Value;
};

struct TSensorValue {
    StringBuf Name;
    TOptionalValue Value;

    bool operator ==(const TSensorValue& other) const {
        return this == &other;
    }

    void operator =(double value) {
        Value = value;
    }

    void Clear() {
        Value.Clear();
    }
};

template <int WindowsSize = 60>
struct TAveragedSensorValue : TSensorValue {
public:
    TAverage<double, WindowsSize> Average;

    void operator =(double value) {
        Average.AddValue(value);
        Value = Average.GetValue();
    }

    void Clear() {
        Average.Clear();
        TSensorValue::Clear();
    }

    void SetValue(double value) {
        Average.SetValue(value);
        Value = value;
    }
};

struct TSensorSource {
    StringBuf Name;
    TTime Updated;

    bool operator ==(const TSensorSource& other) const {
        return this == &other;
    }
};

template <size_t Count> struct TSensor : TSensorSource {
    TSensorValue Values[Count];
};

struct TEventSensorData : TBasicEvent<TEventSensorData> {
    constexpr static TEventID EventID = TEventID::EventSensorData;
    const TSensorSource& Source;
    TSensorValue& Value;

    TEventSensorData(const TSensorSource& source, TSensorValue& value)
        : Source(source)
        , Value(value) {}
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
    TBluetoothActor<TBluetoothZS040> Bluetooth;
    TDigitalPin PowerBluetooth;
    TDigitalPin PowerI2C;
    TLed Led;
    bool Connected = false;
    bool Feed = false;
    static constexpr TTime DefaultPeriod = Env::DefaultPeriod;
    TTime Period = DefaultPeriod;
    TEventReceive* EventReceive;
    TTime LastReportTime;
    TTime ConnectAliveTime;
    TSensorSource TimeSource;
    TSensorValue TimeTotal;
    TSensorValue TimeBusy;
    TSensorValue TimeSleep;

    TSensorActor()
        : TConsoleActor<Env>(this)
        , Channel(this)
        , Bluetooth(this, &Channel)
        , PowerBluetooth(8)
        , PowerI2C(9) {
        Led = true;
        PowerI2C = true;
        PowerBluetooth = true;
        TimeSource.Name = "time";
        TimeTotal.Name = "total";
        TimeBusy.Name = "busy";
        TimeSleep.Name = "sleep";
        delay(10);
        Env::Wire::Begin();
        Led = false;
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
        case TEventSleep::EventID:
            return SensorSleep(static_cast<TEventSleep*>(event.Release()), context);
        case TEventWakeUp::EventID:
            return SensorWakeUp(static_cast<TEventWakeUp*>(event.Release()), context);
        default:
            break;
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
        TimeSleep.Value.SetValue(context.ActorLib.SleepTime.MilliSeconds());
        SendSensorValues(context, TimeSource, TimeTotal, TimeBusy, TimeSleep);
        LastReportTime = context.Now;
    }

    void SensorSleep(TUniquePtr<TEventSleep> event, const TActorContext& context) {
        if (Env::Diagnostics && Env::HaveConsole) {
            context.Send(this, &Console, new TEventData("sleep"));
        }
        PowerBluetooth = false;
        //PowerI2C = false;
        context.ActorLib.Sleeping = true;
    }

    void SensorWakeUp(TUniquePtr<TEventWakeUp> event, const TActorContext& context) {
        context.ActorLib.Sleeping = false;
        if (Env::Diagnostics && Env::HaveConsole) {
            context.Send(this, &Console, new TEventWakeUp());
            context.Send(this, &Console, new TEventData("wakeup"));
        }
        PowerBluetooth = true;
        /*PowerI2C = true;
        delay(10);
        Env::Wire::Begin();*/
        context.Send(this, &Channel, new TEventWakeUp());
    }

    void SensorBootstrap(TUniquePtr<TEventBootstrap> event, const TActorContext& context) {
        Led = true;
        if (Env::HaveConsole) {
            context.ActorLib.Register(&Console);
        }
        context.ActorLib.Register(&Channel);
        context.ActorLib.Register(&Bluetooth);
        if (Env::HaveConsole) {
            context.Send(this, &Console, new TEventData("hi"));
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
        const auto& data(event->Data);
        if (OnCommand(data, context)) {

        } else if (data == "OK") {
        } else if (data == "PING") {
            context.Send(this, event->Sender, new TEventData("PONG"));
        } else if (data == "CONNECTED") {
            Connected = true;
            Feed = false;
            Channel.PurgeEvents(TEventData::EventID);
        } else if (data.starts_with("+")) {
            Connected = false;
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
        } else if (data.starts_with("SLEEP")) {
            if (data.size() > 5) {
                TTime period = TTime::Seconds(data.substr(6));
                context.Send(this, this, new TEventSleep());
                context.Send(this, this, new TEventWakeUp(context.Now + period));
                Feed = false;
                Period = DefaultPeriod;
            }
        } else if (data == "READ") {
            OnSendSensors(context);
            context.Send(this, &Channel, new TEventData("DONE"));
        } else if (data == "STOP") {
            Feed = false;
            Period = DefaultPeriod;
        } else if (data == "RESET") {
            Reset();
        } else if (data.starts_with("BT")) {
            StringBuf command(data);
            command.NextToken(' ');
            context.Send(this, &Channel, new TEventData(command));
        } else {
            context.Send(this, event->Sender, new TEventData("WRONG"));
        }
        if (Env::HaveConsole && event->Sender != &Console) {
            context.Send(this, &Console, event.Release());
        }
        Led = false;
    }

    void SendSensorValue(const TSensorSource& source, TSensorValue& value, const TActorContext& context) {
        if (source.Updated >= LastReportTime && value.Value.IsValid()) {
            StringStream stream;
            stream << "DATA " << source.Name << '.' << value.Name << ' ' << value.Value << " OK";
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
    }

    template <size_t Count>
    void SendSensors(TSensor<Count>& sensor, const TActorContext& context) {
        for (TSensorValue& value : sensor.Values) {
            SendSensorValue(sensor, value, context);
        }
    }

    void SendSensorValues(const TActorContext&, TSensorSource&) {}

    template <typename SensorValue, typename... SensorValues>
    void SendSensorValues(const TActorContext& context, TSensorSource& sensor, SensorValue& value, SensorValues&... values) {
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
            context.Send(this, &Console, new TEventData(StringStream() << "DATA " << event->Source.Name << '.' << event->Value.Name << ' ' << event->Value.Value));
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
        if (LastReportTime + TTime::Minutes(5) < context.Now) {
            Reset();
        }
        if (TTime::Hours(24) < context.Now) {
            Reset();
        }
        if (ConnectAliveTime + TTime::Minutes(3) < context.Now) {
            Reset();
        }
        if (Feed) {
            //ConnectAliveTime = context.Now;
            OnSendSensors(context);
        } else {
            if (!Connected && !context.ActorLib.Sleeping) {
                context.Send(this, &Channel, new TEventData("AT"));
            }
        }
        OnReceive(context);
        event->NotBefore = context.Now + Period;
        context.Resend(this, event.Release());
        delay(100);
        Led = false;
    }

    void Reset() {
        PowerI2C = false;
        PowerBluetooth = false;
        for (int i = 0; i < 15; ++i) {
            Led = true;
            delay(25);
            Led = false;
            delay(50);
        }
        AW::Reset();
    }
};

}
