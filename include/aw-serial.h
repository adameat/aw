#pragma once

#include <HardwareSerial.h>

namespace AW {

#ifdef ARDUINO_ARCH_STM32F1
template <USBSerial& Port, long Baud>
class TUSBSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        //return min((int)Port.available(), 1);
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        //return min(Port.availableForWrite(), 1);
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(reinterpret_cast<const uint8*>(buffer), length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};

template <HardwareSerial& Port, long Baud>
class THardwareSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        //return min((int)Port.available(), 1);
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        //return min(Port.availableForWrite(), 1);
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(reinterpret_cast<const uint8*>(buffer), length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};
#elif ARDUINO_ARCH_SAMD
template <Serial_& Port, long Baud>
class TUSBSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        //return min((int)Port.available(), 1);
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        //return min(Port.availableForWrite(), 1);
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};

template <Uart& Port, long Baud>
class THardwareSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        //return min((int)Port.available(), 1);
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        //return min(Port.availableForWrite(), 1);
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};
#else
template <HardwareSerial& Port, long Baud>
class THardwareSerial {
public:
    static void Begin() {
        Port.begin(Baud);
    }

    static int AvailableForRead() {
        //return min((int)Port.available(), 1);
        return (int)Port.available();
    }

    static int AvailableForWrite() {
        //return min(Port.availableForWrite(), 1);
        return Port.availableForWrite();
    }

    static int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    static int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};
#ifndef Serial1
#define Serial1 Serial
#endif
#endif

template <typename SerialType>
class TSerialActor : public TActor {
public:
    static constexpr String::size_type MaxBufferSize = 128;
    SerialType Port;

    TSerialActor(TActor* owner)
        : Owner(owner)
        , EOL("\n")
    {
        Port.Begin();
    }

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
        case TEventWakeUp::EventID:
            return OnWakeUp(static_cast<TEventWakeUp*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnBootstrap(TUniquePtr<TEventBootstrap>, const TActorContext& context) {
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

    void OnReceive(TUniquePtr<TEventReceive> event, const TActorContext& context) {
        context.Resend(this, event.Release());
        if (context.ActorLib.Sleeping) {
            return;
        }
        String::size_type size = (String::size_type)min(Port.AvailableForRead(), (MaxBufferSize - Buffer.size()));
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

    void OnWakeUp(TUniquePtr<TEventWakeUp>, const TActorContext&) {
        Buffer.clear();
    }
};

template <typename SerialType>
class TSyncSerialActor : public TSerialActor<SerialType> {
public:
    using TBase = TSerialActor<SerialType>;
    TSyncSerialActor(TActor* owner)
        : TBase(owner)
    {}

protected:
    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventData::EventID:
            return OnData(static_cast<TEventData*>(event.Release()), context);
        default:
            return TBase::OnEvent(event, context);
        }
    }

    void OnSend(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventData::EventID:
            return context.ActorLib.SendSync(this, event);
        default:
            return TBase::OnSend(event, context);
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
