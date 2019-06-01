#include <aw.h>
#include "aw-string-buf.h"
#ifdef ARDUINO_ARCH_STM32F1
#include <itoa.h>
#endif
#ifdef ARDUINO_ARCH_SAMD
#include <avr/dtostrf.h>
#endif

namespace AW {

StringPointer::StringPointer(const char* ptr)
    : begin(ptr)
    , length(static_cast<size_type>(strlen(ptr)))
{}

StringBuf::StringBuf(const char* ptr)
    : Begin(ptr)
    , End(ptr + static_cast<size_type>(strlen(ptr)))
{}

bool StringBuf::operator ==(const StringBuf& s) const {
    auto sz = size();
    return sz == s.size() && strncmp(Begin, s.Begin, sz) == 0;
}

StringBuf StringBuf::substr(size_type pos, size_type spos) const {
    size_type sz = size();
    if (spos > sz - pos)
        spos = sz - pos;
    return StringBuf(Begin + pos, Begin + pos + spos);
}

bool StringBuf::starts_with(const StringBuf& s) const {
    size_type sz = s.size();
    return sz <= size() && strncmp(Begin, s.Begin, sz) == 0;
}

StringBuf::size_type StringBuf::find(char c) const {
    for (size_type p = 0; p < size(); ++p) {
        if ((*this)[p] == c) {
            return p;
        }
    }
    return npos;
}

unsigned char StringBuf::touchar() const {
    unsigned char v = 0;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            v = v * 10 + (c - '0');
        } else {
            return 0;
        }
    }
    return v;
}

int StringBuf::toint() const {
    int v = 0;
    bool sign = true;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            v = v * 10 + (c - '0');
        } else if (c == '+') {
            sign = true;
        } else if (c == '-') {
            sign = false;
        } else {
            return int();
        }
    }
    if (sign) {
        return v;
    } else {
        return -v;
    }
}

unsigned long StringBuf::toulong() const {
    unsigned long v = 0;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            v = v * 10 + (c - '0');
        } else {
            return 0;
        }
    }
    return v;
}

long StringBuf::tolong() const {
    long v = 0;
    bool sign = true;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            v = v * 10 + (c - '0');
        } else if (c == '+') {
            sign = true;
        } else if (c == '-') {
            sign = false;
        } else {
            return long();
        }
    }
    if (sign) {
        return v;
    } else {
        return -v;
    }
}

double StringBuf::todouble() const {
    double v = 0;
    unsigned short p = 0;
    bool sign = true;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            unsigned char val = (c - '0');
            if (p > 0) {
                v += ((double)1 / (pow(10, p))) * val;
            } else {
                v = v * 10 + val;
            }
        } else if (c == '.') {
            p = 1;
            continue;
        } else if (c == '+') {
            sign = true;
        } else if (c == '-') {
            sign = false;
        } else {
            return double();
        }
        if (p > 0) {
            ++p;
        }
    }
    if (sign) {
        return v;
    } else {
        return -v;
    }
}

float StringBuf::tofloat() const {
    float v = 0;
    unsigned short p = 0;
    bool sign = true;
    for (char c : *this) {
        if (c <= '9' && c >= '0') {
            unsigned char val = (c - '0');
            if (p > 0) {
                v += ((float)1 / ((float)pow(10, p))) * val;
            } else {
                v = v * 10 + val;
            }
        } else if (c == '.') {
            p = 1;
            continue;
        } else if (c == '+') {
            sign = true;
        } else if (c == '-') {
            sign = false;
        } else {
            return float();
        }
        if (p > 0) {
            ++p;
        }
    }
    if (sign) {
        return v;
    } else {
        return -v;
    }
}

StringBuf StringBuf::NextToken(char delimeter) {
    const char* ptr = Begin;
    while (ptr != End && *ptr == delimeter) {
        ++ptr;
    }
    while (ptr != End && *ptr != delimeter) {
        ++ptr;
    }
    StringBuf result(Begin, ptr);
    while (ptr != End && *ptr == delimeter) {
        ++ptr;
    }
    Begin = ptr;
    return result;
}

uint16_t StringBuf::crc16() const {
    uint16_t crc = 0xffff;
    for (uint8_t c : *this) {
        crc ^= (uint16_t)c;                   // XOR byte into least sig. byte of crc
        for (int i = 8; i != 0; --i) {        // Loop over each bit
            if ((crc & 0x0001) != 0) {        // If the LSB is set
                crc >>= 1;                    // Shift right and XOR 0xA001
                crc ^= 0xA001;
            } else {                          // Else LSB is not set
                crc >>= 1;                    // Just shift right
            }
        }
    }
    return crc;
}

char* StringBuf::dtostrf(double __val, signed char __width, unsigned char __prec, char* __s) {
    char* result = ::dtostrf(__val, __width, __prec, __s);
    auto len = strlen(result);
    --len;
    while (len > 0) {
        char c = result[len];
        if (c != '0' && c != '.') {
            break;
        }
        result[len] = 0;
        --len;
        if (c == '.') {
            break;
        }
    }
    return result;
}

String::String(const char* ptr)
    : StringBuf(ptr, ptr + static_cast<size_type>(strlen(ptr)))
{}

String::String(unsigned int value, unsigned char base)
    : String()
{
    utoa(value, ConversionBuffer, base);
    *this = StringPointer(ConversionBuffer);
}

String::String(int value, unsigned char base)
    : String()
{
    itoa(value, ConversionBuffer, base);
    *this = StringPointer(ConversionBuffer);
}

String::String(unsigned long value, unsigned char base)
    : String()
{
    ultoa(value, ConversionBuffer, base);
    *this = StringPointer(ConversionBuffer);
}

String::String(long value, unsigned char base)
    : String()
{
    ltoa(value, ConversionBuffer, base);
    *this = StringPointer(ConversionBuffer);
}

String::String(float value, unsigned char decimalPlaces)
    : String()
{
    *this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, ConversionBuffer));
}

String::String(double value, unsigned char decimalPlaces)
    : String()
{
    *this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, ConversionBuffer));
}

String::String(const String& string)
    : StringBuf(string.Begin, string.End)
{
    Buffer = string.Buffer;
    if (Buffer != nullptr) {
        ++Buffer->RefCounter;
    }
}

String::String(String&& string) {
    Buffer = string.Buffer;
    Begin = string.Begin;
    End = string.End;
    string.Buffer = nullptr;
    string.Begin = nullptr;
    string.End = nullptr;
}

String& String::operator =(const String& string) {
    Free();
    Begin = string.Begin;
    End = string.End;
    Buffer = string.Buffer;
    if (Buffer != nullptr) {
        ++Buffer->RefCounter;
    }
    return *this;
}

String& String::operator =(String&& string) {
    Free();
    Buffer = string.Buffer;
    Begin = string.Begin;
    End = string.End;
    string.Buffer = nullptr;
    string.Begin = nullptr;
    string.End = nullptr;
    return *this;
}

void String::assign(const char* string, size_type length) {
    resize(length);
    memcpy(Buffer->Data, string, length);
}

void String::append(const char* string, size_type length) {
    resize(size() + length);
    memcpy(const_cast<char*>(End) - length, string, length);
}

void String::erase(size_type pos, size_type length) {
    if (pos == 0 || pos + length == size()) {
        if (pos == 0) {
            Begin += length;
        } else {
            End -= length;
        }
    } else {
        String original = *this;
        resize(size() - length);
        if (pos != 0) {
            memcpy(Buffer->Data, original.begin(), pos);
        }
        if (pos + length != original.size()) {
            memcpy(Buffer->Data + pos, original.begin() + pos + length, original.size() - length - pos);
        }
    }
}

String String::substr(size_type pos, size_type length) const {
    if (length == 0) {
        return String();
    } else {
        return String(*this, pos, length);
    }
}

void String::reserve(size_type length) {
    EnsureOneOwner(length);
}

void String::resize(size_type length) {
    reserve(length);
    End = Begin + length;
}

String::size_type String::capacity() const {
    if (Buffer != nullptr) {
        return static_cast<size_type>(Buffer->end() - begin());
    }
    return 0;
}

bool String::check() const {
    if (end() < begin()) {
        return false;
    }
    if (Buffer != nullptr) {
        if (begin() < Buffer->begin()) {
            return false;
        }
        if (end() > Buffer->end()) {
            return false;
        }
        if (Buffer->Length < size()) {
            return false;
        }
        if (Buffer->RefCounter == 0) {
            return false;
        }
    }
    return true;
}

void String::EnsureOneOwner(size_type size) {
    if (Buffer != nullptr) {
        if (Buffer->RefCounter != 1) {
            const String original = *this;
            size = max((size_type)(16 - sizeof(StringData)), max(size, original.size()));
            Free();
            Alloc(size);
            memcpy(Buffer->Data, original.data(), original.size());
            End = Begin + original.size();
        } else {
            if (Begin == End && Begin != Buffer->Data) {
                End = Begin = Buffer->Data;
            }
            size_type cap = capacity();
            if (size > cap) {
                size = max((size_type)(16 - sizeof(StringData)), size);
                if (Buffer->Length >= size) {
                    if (begin() != Buffer->begin()) {
                        size_type size = this->size();
                        memcpy(Buffer->Data, begin(), size);
                        Begin = Buffer->Data;
                        End = Begin + size;
                    }
                } else {
                    const String original = *this;
                    Free();
                    Alloc(size);
                    memcpy(Buffer->Data, original.data(), original.size());
                    End = Begin + original.size();
                }
            }
        }
    } else {
        if (size != 0) {
            const StringBuf original = *this;
            Alloc(size);
            memcpy(Buffer->Data, original.data(), original.size());
            End = Begin + original.size();
        }
    }
}

String::String(const String& string, size_type pos, size_type length)
    : String() {
    auto size = string.size();
    if (pos + length > size) {
        length = pos > size ? 0 : size - pos;
    }
    if (length != 0) {
        Buffer = string.Buffer;
        if (Buffer != nullptr) {
            ++Buffer->RefCounter;
        }
        Begin = string.Begin + min(pos, size);
        End = Begin + length;
    }
}

void String::Alloc(size_type length) {
    Buffer = reinterpret_cast<StringData*>(new char[length + sizeof(StringData)]);
    Buffer->Length = length;
    Buffer->RefCounter = 1;
    Begin = Buffer->Data;
    End = Begin;
}

void String::Free() {
    if (Buffer != nullptr) {
        if (--Buffer->RefCounter == 0) {
            delete [](reinterpret_cast<char*>(Buffer));
            Buffer = nullptr;
            Begin = End = nullptr;
        }
    }
}

}