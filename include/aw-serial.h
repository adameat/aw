#pragma once

#include <HardwareSerial.h>
#include "aw-debug.h"
#if defined(ARDUINO_ARCH_SAMD)
#include <wiring_private.h>
#endif

namespace AW {

template <typename PortType, PortType& Port, long Baud>
class TBasicSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static void Flush() {
        Port.flush();
    }

    static void SkipAll() {
        int size = AvailableForRead();
        while (size > 0) {
            char dummy;
            Read(&dummy, 1);
            --size;
        }
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};

#ifdef ARDUINO_ARCH_STM32F1
template <USBSerial& Port, long Baud, uint8_t RX = 0, uint8_t TX = 1>
class TUSBSerial : public TBasicSerial<USBSerial, Port, Baud> {
public:
    static int Write(const char* buffer, int length) {
        return Port.write(reinterpret_cast<const uint8*>(buffer), length);
    }
};

template <HardwareSerial& Port, long Baud, uint8_t RX = 0, uint8_t TX = 1>
class THardwareSerial : public TBasicSerial<HardwareSerial, Port, Baud> {
public:
    static int Write(const char* buffer, int length) {
        return Port.write(reinterpret_cast<const uint8*>(buffer), length);
    }
};
#elif ARDUINO_ARCH_SAMD
template <Serial_& Port, long Baud>
class TUSBSerial : public TBasicSerial<Serial_, Port, Baud> {
public:
};

template <Uart& Port, long Baud, uint8_t RX = 0, uint8_t TX = 1>
class THardwareSerial : public TBasicSerial<Uart, Port, Baud> {
public:
    static void Begin() {
        TBasicSerial<Uart, Port, Baud>::Begin();
        if (RX != 0) {
            pinPeripheral(RX, PIO_SERCOM);
        }
        if (TX != 1) {
            pinPeripheral(TX, PIO_SERCOM);
        }
    }
};
#elif ARDUINO_ARCH_NRF5
template <Uart& Port, long Baud>
class THardwareSerial : public TBasicSerial<Uart, Port, Baud> {
public:
    static void Begin() {
        TBasicSerial<Uart, Port, Baud>::Begin();
    }
};
#else
template <HardwareSerial& Port, long Baud, uint8_t RX = 0, uint8_t TX = 1>
class THardwareSerial : public TBasicSerial<HardwareSerial, Port, Baud> {
public:
};
#endif
#ifndef Serial1
#define Serial1 Serial
#endif

template <typename SerialType>
class TSerialActor : public TActor {
public:
    static constexpr String::size_type MaxBufferSize = 128;
    SerialType Port;

    TSerialActor(TActor* owner)
        : Owner(owner)
        , EOL("\n")
    {}

protected:
    TActor* Owner;
    String Buffer;
    StringBuf EOL;

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventBootstrap::EventID:
            return OnBootstrap(static_cast<TEventBootstrap*>(event.Release()), context);
        case TEventData::EventID:
            return OnData(static_cast<TEventData*>(event.Release()), context);
        /*case TEventDataArray::EventID:
            return OnData(static_cast<TEventDataArray*>(event.Release()), context);*/
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
        Port.Begin();
        context.Send(this, this, new TEventReceive);
    }

	/*static constexpr TTime GetSafeIdleTime() {
		return TTime::MilliSeconds(ArduinoSettings::GetReceiveBufferSize() * 1000 / (SerialType::GetBaud() / 8));
	}*/

    void OnData(TUniquePtr<TEventData> event, const TActorContext& context) {
        String::size_type availableForWrite = (String::size_type)Port.AvailableForWrite();
        if (availableForWrite > 0) {
            String& data = event->Data;
            String::size_type len = data.size();
            if (len + EOL.size() <= availableForWrite) {
                if (len > 0) {
                    Port.Write(data.begin(), len);
                }
                Port.Write(EOL.data(), EOL.size());
                return;
            } else {
                Port.Write(data.begin(), availableForWrite);
                data.erase(0, availableForWrite);
            }
        }
        context.ResendImmediate(this, event.Release());
    }

    bool Sleeping = false;

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        // TODO: disable for sleep
        if (context.ActorLib.Sleeping) {
            Sleeping = true;
            return;
        }
        context.Resend(this, event.Release());
        String::size_type size = (String::size_type)min((unsigned int)Port.AvailableForRead(), (MaxBufferSize - Buffer.size()));
        if (size > 0) {
            String::size_type strStart = 0;
            String::size_type bufferPos = Buffer.size();
            Buffer.reserve(bufferPos + size);
            size = (String::size_type)Port.Read(Buffer.data() + bufferPos, size);
            String::size_type bufferSize = bufferPos + size;
            Buffer.resize(bufferSize);
            while (bufferPos < bufferSize) {
                if (Buffer[bufferPos] == '\n') {
                    String::size_type strSize = bufferPos;
                    while (strSize > 0 && Buffer[strSize - 1] == '\r') {
                        --strSize;
                    }
                    context.Send(this, Owner, new TEventData(Buffer.substr(strStart, strSize - strStart)));
                    strStart = bufferPos + 1;
                }
                ++bufferPos;
            }
            Buffer = Buffer.substr(strStart, bufferPos - strStart);
            if (bufferSize >= MaxBufferSize) {
                context.Send(this, Owner, new TEventData(Buffer));
                Buffer.clear();
            }
        }
        //context.ResendAfter(this, event.Release(), GetSafeIdleTime());
    }

    void OnSleep(TUniquePtr<TEventSleep>, const TActorContext&) {
        Port.Flush();
    }

    void OnWakeUp(TUniquePtr<TEventWakeUp>, const TActorContext& context) {
        Buffer.clear();
        Port.SkipAll();
        if (Sleeping) {
            context.Send(this, this, new TEventReceive);
            Sleeping = false;
        }
    }
};

template <typename SerialType>
class TSyncSerialActor : public TSerialActor<SerialType> {
public:
    using TBase = TSerialActor<SerialType>;
    TSyncSerialActor(TActor* owner)
        : TBase(owner)
    {}

    void OnSend(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventData::EventID:
            return context.ActorLib.SendSync(this, Move(event));
        default:
            return TBase::OnSend(Move(event), context);
        }
    }

protected:
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventData::EventID:
            return OnData(static_cast<TEventData*>(event.Release()), context);
        default:
            return TBase::OnEvent(Move(event), context);
        }
    }

    void OnData(TUniquePtr<TEventData> event, const TActorContext&) {
        String& data = event->Data;
        int len = data.size();
        if (len > 0) {
            TBase::Port.Write(data.begin(), len);
        }
        TBase::Port.Write(TBase::EOL.data(), TBase::EOL.size());
    }
};

}
