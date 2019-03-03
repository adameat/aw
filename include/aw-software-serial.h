#pragma once

#include <SoftwareSerial.h>

namespace AW {

template <int RxPin, int TxPin, long Baud>
class TSoftwareSerial {
protected:
    SoftwareSerial Port;

public:
    TSoftwareSerial()
        : Port(RxPin, TxPin)
    {}

    void Begin() {
        Port.begin(Baud);
    }

    int AvailableForRead() const {
        return const_cast<SoftwareSerial&>(Port).available();
    }

    int AvailableForWrite() const {
        return 32;
    }

    int Write(const char* buffer, int length) {
        return Port.write(buffer, length);
    }

    int Read(char* buffer, int length) {
        return Port.readBytes(buffer, length);
    }

    static constexpr long GetBaud() {
        return Baud;
    }
};

}
