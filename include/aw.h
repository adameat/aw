#pragma once

#define _DEBUG

#ifdef _DEBUG
//#define _DEBUG_HANG_ON_RESET
#define _DEBUG_HEAP
#define _DEBUG_SLEEP
#define _DEBUG_WATCHDOG
#endif

#include <Arduino.h>
#include <Wire.h>
#include "aw-string-buf.h"
#include "aw-stream.h"
#include "aw-average.h"
#include "aw-functional.h"
#include "aw-pointer.h"
#include "aw-vector.h"

namespace AW {
    extern void(*Reset)(StringBuf reason);
}

#include "aw-dequeue.h"
#include "aw-list.h"
#include "aw-time.h"

namespace AW {

class ArduinoSettings {
public:
    static constexpr float GetReferenceVoltage() {
#ifdef ARDUINO_ARCH_AVR
#if F_CPU == 8000000L
        return 3.3;
#else
        return 5.0; // TODO: for now
#endif
#endif
#ifdef ARDUINO_ARCH_SAMD
        return 3.3;
#endif
#ifdef ARDUINO_ARCH_STM32F1
        return 3.3;
#endif
#ifdef ARDUINO_ARCH_NRF5
        return 3.3;
#endif
    }

    static constexpr unsigned long GetReadResolution() {
#ifdef ARDUINO_ARCH_AVR
        return 1024; // TODO: for now
#endif
#ifdef ARDUINO_ARCH_SAMD
        return 4096;
#endif
#ifdef ARDUINO_ARCH_STM32F1
        return 4096;
#endif
#ifdef ARDUINO_ARCH_NRF5
        return 4096;
#endif
    }

    static constexpr unsigned long GetWriteResolution() {
#ifdef ARDUINO_ARCH_AVR
        return 256;
#endif
#ifdef ARDUINO_ARCH_SAMD
        return 256; // TODO: for now
#endif
#ifdef ARDUINO_ARCH_STM32F1
        return 65536;
#endif
#ifdef ARDUINO_ARCH_NRF5
        return 256;
#endif
    }

	static constexpr unsigned long GetReceiveBufferSize() {
		return 64;
	}

	static constexpr unsigned long GetSendBufferSize() {
		return 64;
	}
};

#ifdef ARDUINO_ARCH_AVR
#endif

#ifdef ARDUINO_ARCH_STM32F1
#define LED_BUILTIN PC13
#endif

enum TEventID : uint8_t {
    EventBootstrap,
    EventReceive,
    EventData,
    EventSensorData,
    EventSensorMessage,
    EventScheduledFunction,
    EventSleep,
    EventWakeUp,
    EventPrivate0,
    EventPrivate1,
    EventPrivate2,
    EventPrivate3,
    EventPrivate4,
    EventPrivate5,
    EventPrivate6,
    EventPrivate7,
    EventPrivate8,
    EventPrivate9,
};

class TActor;
struct TEvent;
class TActorLib;

struct TActorContext;

using TActorPtr = TActor*;
using TEventPtr = TUniquePtr<TEvent>;

class TActor {
public:
    friend class TActorLib;
    TActor* NextActor = nullptr;
    TTime BusyTime;
    TList<TEventPtr> Events;

    virtual void OnEvent(TEventPtr event, const TActorContext& context) = 0;
    virtual void OnSend(TEventPtr event, const TActorContext& context);
    void PurgeEvents(TEventID eventId);
};

struct TEvent : TList<TUniquePtr<TEvent>>::TItemBase {
    TTime NotBefore;
    TActor* Sender;
    TEventID EventID;
};

template <typename DerivedType>
struct TBasicEvent : TEvent {
    TBasicEvent() {
        TEvent::EventID = DerivedType::EventID;
    }
};

class TDummyActor : public TActor {
public:
    void OnEvent(TEventPtr, const TActorContext&) override {}
};

class TActorLib {
public:
    TTime MinSleepPeriod = TTime::MilliSeconds(1);
    TTime MaxSleepPeriod = TTime::MilliSeconds(4000);
    TTime WatchdogTimeout = TTime::MilliSeconds(8000);
    TTime BusyTime;
    TTime SleepTime;
    bool Sleeping = false;

    TActorLib();
    void Register(TActor* actor, TTime drift = TTime());
    void Run();
    void Send(TActor* recipient, TEventPtr event);
    void SendImmediate(TActor* recipient, TEventPtr event);
    void SendSync(TActor* recipient, TEventPtr event);
    void Resend(TActor* recipient, TEventPtr event);
    void ResendImmediate(TActor* recipient, TEventPtr event);
    void Sleep();
    void WakeUp();

protected:
    //TDeque<TEventPtr, 16> Events;

    TActor* Actors = nullptr;
    // TDeque<TEventPtr> with different sizes in every actor
    // or maybe dynamic TDeque<TEventPtr> ?
    // mailbox should be inside every actor for faster sending
};

struct TActorContext {
    TActorLib& ActorLib;
    TTime Now;

    TActorContext(TActorLib& actorLib);
    void Send(TActor* sender, TActor* recipient, TEventPtr event) const;
    void SendAfter(TActor* sender, TActor* recipient, TEventPtr event, TTime time) const;
    void SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) const;
    void Resend(TActor* recipient, TEventPtr event) const;
    void ResendImmediate(TActor* recipient, TEventPtr event) const;
    void ResendAfter(TActor* recipient, TEventPtr event, TTime time) const;
};

struct TEventScheduledFunction : TBasicEvent<TEventScheduledFunction> {
    constexpr static TEventID EventID = TEventID::EventScheduledFunction;
    TFunction Function;

    template <typename T>
    TEventScheduledFunction(AW::TTime notBefore, T lambda)
        : Function(Move(lambda))
    {
        NotBefore = notBefore;
    }
};

class TSchedulerActor : public TActor {
public:
    void OnEvent(TEventPtr event, const TActorContext&) override {
        switch (event->EventID) {
        case TEventScheduledFunction::EventID:
            static_cast<TEventScheduledFunction*>(event.Release())->Function();
            break;
        default:
            break;
        }
    }

    template <typename lambda>
    void Schedule(const TActorContext& context, TTime time, lambda function) {
        context.Send(this, this, new TEventScheduledFunction(time, function));
    }
};

//template <uint8_t P, uint8_t Mode = OUTPUT>
//class TPin {
//public:
//    TPin() {
//        pinMode(P, Mode);
//    }
//
//    TPin& operator =(bool state) {
//        digitalWrite(P, state ? HIGH : LOW);
//        return *this;
//    }
//
//    operator bool() const {
//        return digitalRead(P) == HIGH;
//    }
//
//    TPin& operator =(int value) {
//        analogWrite(P, value);
//        return *this;
//    }
//
//    operator int() const {
//        return analogRead(P);
//    }
//
//    static float GetValue() {
//        int value = analogRead(P);
//        return ArduinoSettings::GetReferenceVoltage() * value / (ArduinoSettings::GetReadResolution() - 1);
//    }
//
//    template <int Iterations = 100, unsigned long Delay = 0>
//    static float GetAveragedValue() {
//        TAverage<float, Iterations> value;
//        for (int i = 0; i < Iterations; ++i) {
//            if (i != 0) {
//                delay(Delay);
//            }
//            value.AddValue(GetValue());
//        }
//        return value.GetValue();
//    }
//
//    static void SetValue(float value) {
//        int v = value * (ArduinoSettings::GetWriteResolution() - 1);
//        analogWrite(P, v);
//    }
//
//    static void SetMode(uint8_t mode) {
//        pinMode(P, mode);
//    }
//};

#ifdef ARDUINO_ARCH_AVR
template <uint16_t PORT, uint16_t DDR, uint16_t PIN>
class TArduinoPort {
public:
    static void DigitalWrite(uint8_t bit, bool value) {
        if (value) {
            *((unsigned char*)PORT) |= 1 << bit;
        } else {
            *((unsigned char*)PORT) &= ~(1 << bit);
        }
    }

    static void PinMode(uint8_t bit, bool output) {
        if (output) {
            *((unsigned char*)DDR) |= 1 << bit;
        } else {
            *((unsigned char*)DDR) &= ~(1 << bit);
        }
    }
};

using TArduinoPortB = TArduinoPort<0x37, 0x36, 0x35>;

template <typename ArduinoPort, uint8_t BIT>
class TArduinoDigitalPin : private ArduinoPort {
public:
    static void DigitalWrite(bool value) {
        ArduinoPort::DigitalWrite(BIT, value);
    }

    void operator =(bool value) {
        DigitalWrite(value);
    }
};
#endif

template <uint8_t PIN>
class TPin {
public:
#ifdef ARDUINO_ARCH_STM32F1
    using ModeType = WiringPinMode;
#else
    using ModeType = uint8_t;
#endif
    TPin(ModeType mode = OUTPUT)
    {
        pinMode(PIN, mode);
    }

    void SetMode(ModeType mode) {
        pinMode(PIN, mode);
    }

    uint8_t GetPin() const {
        return PIN;
    }
};

template <>
class TPin<0> {
public:
#ifdef ARDUINO_ARCH_STM32F1
    using ModeType = WiringPinMode;
#else
    using ModeType = uint8_t;
#endif
    TPin(ModeType mode = OUTPUT)
    {}

    void SetMode(ModeType mode) {}

    uint8_t GetPin() const {
        return 0;
    }
};

template <uint8_t PIN>
class TAnalogPin : public TPin<PIN> {
public:
    using ModeType = typename TPin<PIN>::ModeType;

    TAnalogPin(ModeType mode = OUTPUT)
        : TPin<PIN>(mode)
    {}

    TAnalogPin& operator =(uint16_t value) {
        analogWrite(PIN, value);
        return *this;
    }

    operator uint16_t() const {
        return analogRead(PIN);
    }

    float GetValue() {
        uint16_t value = analogRead(PIN);
        return (float)value / (ArduinoSettings::GetReadResolution() - 1);
    }

    float GetVoltage() {
        return ArduinoSettings::GetReferenceVoltage() * GetValue();
    }

    template <int Iterations = 100, unsigned long Delay = 0>
    float GetAveragedValue() {
        TAverage<float, Iterations> value;
        for (int i = 0; i < Iterations; ++i) {
            if (i != 0) {
                delay(Delay);
            }
            value.AddValue(GetValue());
        }
        return value.GetValue();
    }
};

template <uint8_t PIN>
class TPWMPin : public TPin<PIN> {
public:
    using ModeType = typename TPin<PIN>::ModeType;
#ifdef ARDUINO_ARCH_STM32F1
    TPWMPin(ModeType mode = PWM)
#else
    TPWMPin(ModeType mode = OUTPUT)
#endif
        : TPin<PIN>(mode)
    {}

    TPWMPin& operator =(uint16_t value) {
#ifdef ARDUINO_ARCH_STM32F1
        pwmWrite(PIN, value);
#else
        analogWrite(PIN, value);
#endif
        return *this;
    }

    void SetValue(float value) {
        uint16_t v = uint16_t(value * (ArduinoSettings::GetWriteResolution() - 1));
#ifdef ARDUINO_ARCH_STM32F1
        pwmWrite(PIN, v);
#else
        analogWrite(PIN, v);
#endif
    }
};

template <uint8_t PIN>
class TDigitalPin : public TPin<PIN> {
public:
    using ModeType = typename TPin<PIN>::ModeType;

    TDigitalPin(ModeType mode = OUTPUT)
        : TPin<PIN>(mode)
    {}

    TDigitalPin& operator =(bool state) {
        digitalWrite(PIN, state ? HIGH : LOW);
        return *this;
    }

    operator bool() const {
        return digitalRead(PIN) == HIGH;
    }

    unsigned long PulseIn(uint8_t mode = HIGH) {
        return pulseIn(PIN, mode);
    }
};

template <>
class TDigitalPin<0> : public TPin<0> {
public:
    using ModeType = typename TPin<0>::ModeType;

    TDigitalPin(ModeType mode = OUTPUT)
        : TPin<0>(mode)
    {}

    TDigitalPin& operator =(bool state) {
        return *this;
    }

    operator bool() const {
        return false;
    }

    unsigned long PulseIn(uint8_t mode = HIGH) {
        return 0;
    }
};

class TLed : public TPin<LED_BUILTIN> {
public:
    TLed()
    {}

    TLed& operator =(bool state) {
#ifdef ARDUINO_ARCH_STM32F1
        digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
#else
        digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
#endif
        return *this;
    }
};

/*template <typename Type, int WindowSize = 10>
class TAveragedValue {
protected:
    Type Value;
    using ValueType = decltype(Value.GetValue());
    mutable TAverage<ValueType, WindowSize> Average;

public:
    ValueType GetValue() const {
        Average.AddValue(Value.GetValue());
        return Average.GetValue();
    }
};*/

class TPeriodicTrigger {
public:
    bool IsTriggered(TTime period, const TActorContext& context) {
        if (LastTriggered + period <= context.Now) {
            LastTriggered = context.Now;
            return true;
        }
        return false;
    }

protected:
    TTime LastTriggered;
};

template <typename Wire>
class TWireDevice {
public:
    class TWireLocation {
    public:
        TWireLocation(uint8_t address, uint8_t location)
            : Address(address)
            , Location(location)
        {}

        template <typename Type>
        void operator =(Type value) {
            Wire::BeginTransmission(Address);
            Wire::Write(Location);
            for (size_t i = 0; i < sizeof(Type); ++i) {
                Wire::Write(reinterpret_cast<uint8_t*>(&value)[i]);
            }
            Wire::EndTransmission();
        }

        template <typename Type>
        operator Type() {
            Type result;
            Wire::BeginTransmission(Address);
            Wire::Write(Location);
            Wire::EndTransmission();
            Wire::RequestFrom(Address, sizeof(Type));
            for (size_t i = 0; i < sizeof(Type); ++i) {
                Wire::Read(reinterpret_cast<uint8_t*>(&result)[i]);
            }
            return result;
        }

    protected:
        uint8_t Address;
        uint8_t Location;
    };

    TWireDevice(uint8_t address)
        : Address(address)
    {}

    TWireLocation operator [](uint8_t location) {
        return TWireLocation(Address, location);
    }

protected:
    uint8_t Address;
};

class TWire {
public:
    static void Begin() { Wire.begin(); }
    static void BeginTransmission(uint8_t address) { Wire.beginTransmission(address); }
    static void Write(uint8_t value) { Wire.write(value); }
    static void Write(uint16_t value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Wire.write(values[1]); Wire.write(values[0]); }
    static bool EndTransmission(bool stop = true) { return Wire.endTransmission(stop) == 0; }
    static uint8_t RequestFrom(uint8_t address, uint8_t quantity) { return Wire.requestFrom(address, quantity); }
    static void Read(uint8_t& value) { value = Wire.read(); }
    static void Read(int8_t& value) { Read(reinterpret_cast<uint8_t&>(value)); }
    static void Read(uint16_t& value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Read(values[1]); Read(values[0]); }
    static void Read(int16_t& value) { Read(reinterpret_cast<uint16_t&>(value)); }
    static void ReadLE(uint16_t& value) { uint8_t* values = reinterpret_cast<uint8_t*>(&value); Read(values[0]); Read(values[1]); }
    static void ReadLE(int16_t& value) { ReadLE(reinterpret_cast<uint16_t&>(value)); }

    template <typename T>
    static bool ReadValue(uint8_t addr, uint8_t reg, T& val) {
        BeginTransmission(addr);
        Write(reg);
        if (!EndTransmission())
            return false;
        if (RequestFrom(addr, sizeof(T)) != sizeof(T))
            return false;
        Read(val);
        return true;
    }

    template <typename T>
    static bool ReadValueLE(uint8_t addr, uint8_t reg, T& val) {
        BeginTransmission(addr);
        Write(reg);
        if (!EndTransmission())
            return false;
        if (RequestFrom(addr, sizeof(T)) != sizeof(T))
            return false;
        ReadLE(val);
        return true;
    }

    template <typename T>
    static bool WriteValue(uint8_t addr, uint8_t reg, T val) {
        BeginTransmission(addr);
        Write(reg);
        Write(val);
        return EndTransmission();
    }

    template <typename T>
    static void Read(T& value) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&value);
        auto length = sizeof(value);
        while (length-- > 0) {
            Read(*data);
            ++data;
        }
    }

    template <typename T>
    static void ReadLE(T& value) {
        uint8_t* data = reinterpret_cast<uint8_t*>(&value);
        auto length = sizeof(value);
        data += length;
        while (length-- > 0) {
            --data;
            Read(*data);
        }
    }

    static TWireDevice<TWire> GetDevice(uint8_t address) {
        return TWireDevice<TWire>(address);
    }
};

struct uint24_t {
    uint8_t data[3];

    uint24_t() = default;

    uint24_t(uint32_t v) {
        data[2] = (v >> 16) & 0xff;
        data[1] = (v >> 8) & 0xff;
        data[0] = v & 0xff;
    }

    operator uint32_t() const {
        uint32_t v = data[0];
        v |= ((uint32_t)data[1]) << 8;
        v |= ((uint32_t)data[2]) << 16;
        return v;
    }
};

inline void swap(uint8_t& a, uint8_t& b) {
    uint8_t c = a;
    a = b;
    b = c;
}

inline uint16_t bswap(uint16_t v) {
    union u {
        struct {
            uint8_t l;
            uint8_t h;
        } u8;
        uint16_t u16;
    };
    u t;
    t.u16 = v;
    swap(t.u8.l, t.u8.h);
    return t.u16;
}

void DefaultReset(StringBuf reason);
StringBuf GetLastResetReason();

} // namespace AW

namespace AW {

struct TEventBootstrap : TBasicEvent<TEventBootstrap> {
    constexpr static TEventID EventID = TEventID::EventBootstrap;
};

struct TEventReceive : TBasicEvent<TEventReceive> {
    constexpr static TEventID EventID = TEventID::EventReceive;

    TEventReceive() = default;

    TEventReceive(TTime notBefore) {
        NotBefore = notBefore;
    }
};

struct TEventData : TBasicEvent<TEventData> {
    constexpr static TEventID EventID = TEventID::EventData;
    String Data;
    TEventData(const String& data);
};

struct TEventSleep : TBasicEvent<TEventSleep> {
    constexpr static TEventID EventID = TEventID::EventSleep;
    TEventSleep() = default;
};

struct TEventWakeUp : TBasicEvent<TEventWakeUp> {
    constexpr static TEventID EventID = TEventID::EventWakeUp;
    TEventWakeUp() = default;
    TEventWakeUp(TTime notBefore) {
        NotBefore = notBefore;
    }
};

} // namespace AW

#include "aw-led.h"
#include "aw-memory.h"
#include "aw-serial.h"
#include "aw-fixed.h"

namespace AW {

struct TDefaultEnvironment {
    // debug info
    static constexpr bool Diagnostics = false;

    // sensors
    static constexpr TTime DefaultPeriod = TTime::Seconds(30);
    static constexpr TTime SensorsPeriod = TTime::MilliSeconds(5000);
    static constexpr TTime WarmupPeriod = TTime::Seconds(10);
    static constexpr bool SensorsSendValues = false;
    static constexpr bool SensorsCalibration = false;

    using Wire = TWire;
    static constexpr bool HaveConsole = false;
    static constexpr bool DumpSensorData = false;
    static constexpr bool SupportsSleep = true;

    static constexpr bool UseSum = false;
    static constexpr bool UseCRC16 = true;

    static constexpr int BluetoothBaudRate = 9600;
    static constexpr int AverageSensorWindow = 60;

    // pins
    static constexpr uint8_t PIN_POWER_BLUETOOTH = 8;
    static constexpr uint8_t PIN_POWER_I2C = 0; // 9
    static constexpr uint8_t PIN_LED_SLEEP = 0; // 7

#if defined(ARDUINO_ARCH_STM32F1)
    using ConsoleActor = TSyncSerialActor<TUSBSerial<Serial, 9600>>;
#elif defined(ARDUINO_ARCH_SAMD)
    using ConsoleActor = TSyncSerialActor<TUSBSerial<SerialUSB, 9600>>;
#else
    using ConsoleActor = TSyncSerialActor<THardwareSerial<Serial, 9600>>;
#endif
};

} // namespace AW
