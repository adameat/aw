#pragma once
#include <string.h>
#ifdef ARDUINO_ARCH_STM32F1
#include <itoa.h>
#endif
#ifdef ARDUINO_ARCH_SAMD
#include <avr/dtostrf.h>
#endif

namespace AW {

struct StringPointer {
    using size_type = uint8_t;
    static constexpr size_type npos = 0xff;

    StringPointer(const char* ptr)
        : begin(ptr)
        , length(static_cast<size_type>(strlen(ptr)))
    {}

    const char* begin;
    size_type length;
};

class StringBuf {
public:
    using size_type = StringPointer::size_type;
    constexpr static size_type npos = 0xff;

    constexpr StringBuf(const char* begin, size_type length)
        : Begin(begin)
        , End(begin + length) {}

    constexpr StringBuf(const char* begin, const char* end)
        : Begin(begin)
        , End(end) {}

    template <size_type N>
    constexpr StringBuf(const char(&string)[N])
        : StringBuf(&string[0], &string[N - 1])
    {}

    StringBuf(StringPointer ptr)
        : Begin(ptr.begin)
        , End(ptr.begin + ptr.length)
    {}

    StringBuf(const char* ptr)
        : Begin(ptr)
        , End(ptr + static_cast<size_type>(strlen(ptr)))
    {}

    constexpr StringBuf()
        : Begin(nullptr)
        , End(nullptr) {}

    size_type size() const {
        return static_cast<size_type>(End - Begin);
    }

    size_type length() const {
        return size();
    }

    bool empty() const {
        return Begin == End;
    }

    bool operator ==(const StringBuf& s) const {
        auto sz = size();
        return sz == s.size() && strncmp(Begin, s.Begin, sz) == 0;
    }

    bool operator !=(const StringBuf& s) const {
        return !operator ==(s);
    }

    const char* data() const {
        return Begin;
    }

    const char* begin() const {
        return Begin;
    }

    const char* end() const {
        return End;
    }

    StringBuf substr(size_type pos, size_type spos = npos) const {
        size_type sz = size();
        if (spos > sz - pos)
            spos = sz - pos;
        return StringBuf(Begin + pos, Begin + pos + spos);
    }

    bool starts_with(const StringBuf& s) const {
        size_type sz = s.size();
        return sz <= size() && strncmp(Begin, s.Begin, sz) == 0;
    }

    char operator [](size_type pos) const {
        return *(Begin + pos);
    }

    size_type find(char c) const {
        for (size_type p = 0; p < size(); ++p) {
            if ((*this)[p] == c) {
                return p;
            }
        }
        return npos;
    }

    operator unsigned char() const {
        return touchar();
    }

    unsigned char touchar() const {
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

    operator int() const {
        return toint();
    }

    int toint() const {
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

	operator unsigned long() const {
		return toulong();
	}

	unsigned long toulong() const {
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

	operator long() const {
		return tolong();
	}

	long tolong() const {
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

	operator double() const {
		return todouble();
	}

	double todouble() const {
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

    operator float() const {
        return tofloat();
    }

    float tofloat() const {
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

    StringBuf NextToken(char delimeter = ' ') {
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

    uint16_t crc16() const {
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

protected:
    const char* Begin;
    const char* End;
};

class String : public StringBuf {
public:
    constexpr String() = default;

    String(const char* begin, size_type length)
        : String()
    {
        assign(begin, length);
    }

    String(const char* begin, const char* end)
        : String(begin, static_cast<size_type>(end - begin))
    {}

    template <size_type N>
    constexpr String(const char(&string)[N])
        : StringBuf(&string[0], &string[N - 1])
    {}

    String(const char* ptr)
        : StringBuf(ptr, ptr + static_cast<size_type>(strlen(ptr)))
    {}

    static char ConversionBuffer[32];

    String(StringPointer ptr)
        : String(ptr.begin, ptr.length)
    {}

    String(StringBuf buf)
        : String(buf.begin(), buf.size())
    {}

    String(unsigned int value, unsigned char base = 10)
        : String()
    {
        utoa(value, ConversionBuffer, base);
        *this = StringPointer(ConversionBuffer);
    }

    String(int value, unsigned char base = 10)
        : String()
    {
        itoa(value, ConversionBuffer, base);
        *this = StringPointer(ConversionBuffer);
    }

    String(unsigned long value, unsigned char base = 10)
        : String()
    {
        ultoa(value, ConversionBuffer, base);
        *this = StringPointer(ConversionBuffer);
    }

    String(long value, unsigned char base = 10)
        : String()
    {
        ltoa(value, ConversionBuffer, base);
        *this = StringPointer(ConversionBuffer);
    }

    String(float value, unsigned char decimalPlaces = 2)
        : String()
    {
        *this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, ConversionBuffer));
    }

    String(double value, unsigned char decimalPlaces = 2)
        : String()
    {
        *this = StringPointer(dtostrf(value, (decimalPlaces + 2), decimalPlaces, ConversionBuffer));
    }

    String(const String& string)
        : StringBuf(string.Begin, string.End)
    {
        Buffer = string.Buffer;
        if (Buffer != nullptr) {
            ++Buffer->RefCounter;
        }
    }

    String(String&& string) {
        Buffer = string.Buffer;
        Begin = string.Begin;
        End = string.End;
        string.Buffer = nullptr;
        string.Begin = nullptr;
        string.End = nullptr;
    }

    String& operator =(const String& string) {
        Free();
        Begin = string.Begin;
        End = string.End;
        Buffer = string.Buffer;
        if (Buffer != nullptr) {
            ++Buffer->RefCounter;
        }
        return *this;
    }

    String& operator =(String&& string) {
        Free();
        Buffer = string.Buffer;
        Begin = string.Begin;
        End = string.End;
        string.Buffer = nullptr;
        string.Begin = nullptr;
        string.End = nullptr;
        return *this;
    }

    String& operator +=(const StringBuf& string) {
        append(string.data(), string.size());
        return *this;
    }

    String& operator +=(char string) {
        return *this += StringBuf(&string, 1);
    }

    String operator +(const StringBuf& string) const {
        String result(*this);
        result += string;
        return result;
    }

    String operator +(char string) const {
        String result(*this);
        result += string;
        return result;
    }

    ~String() {
        Free();
    }

    void assign(const char* string, size_type length) {
        resize(length);
        memcpy(Buffer->Data, string, length);
    }

    void append(const char* string, size_type length) {
        resize(size() + length);
        memcpy(const_cast<char*>(End) - length, string, length);
    }

    void erase(size_type pos, size_type length) {
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

    String substr(size_type pos, size_type length = npos) const {
        if (length == 0) {
            return String();
        } else {
            return String(*this, pos, length);
        }
    }

    void reserve(size_type length) {
        EnsureOneOwner(length);
    }

    void resize(size_type length) {
        reserve(length);
        End = Begin + length;
    }

    void clear() {
        resize(0);
    }

    const char* data() const {
        return Begin;
    }

    char* data() {
        EnsureOneOwner();
        return const_cast<char*>(Begin);
    }

    size_type capacity() const {
        if (Buffer != nullptr) {
            return static_cast<size_type>(Buffer->end() - begin());
        }
        return 0;
    }

    bool check() const {
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

    bool _IsShared() const { return Buffer == nullptr || Buffer->RefCounter > 1; }
    bool _IsUnique() const { return Buffer != nullptr && Buffer->RefCounter == 1; }

protected:
    void EnsureOneOwner(size_type size = 0) {
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

    String(const String& string, size_type pos, size_type length)
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

#ifndef ARDUINO
#pragma warning(disable:4200)
#endif
    struct StringData {
        size_type Length;
        unsigned char RefCounter;
        char Data[];

        char* begin() {
            return Data;
        }

        char* end() {
            return begin() + Length;
        }
    };
#ifndef ARDUINO
#pragma warning(default:4200)
#endif

    void Alloc(size_type length) {
        Buffer = (StringData*)malloc(length + sizeof(StringData));
        Buffer->Length = length;
        Buffer->RefCounter = 1;
        Begin = Buffer->Data;
        End = Begin;
    }

    void Free() {
        if (Buffer != nullptr) {
            if (--Buffer->RefCounter == 0) {
                free(Buffer);
                Buffer = nullptr;
                Begin = End = nullptr;
            }
        }
    }

    StringData* Buffer = nullptr;
};

class StringStream {
public:
    /*StringStream& operator <<(String string) {
        Data += string;
        return *this;
    }*/

    StringStream& operator <<(StringBuf string) {
        Data += string;
        return *this;
    }

    StringStream& operator <<(char string) {
        Data += string;
        return *this;
    }

    StringStream& operator <<(int string) {
        Data += String(string);
        return *this;
    }

    StringStream& operator <<(unsigned int string) {
        Data += String(string);
        return *this;
    }

    StringStream& operator <<(long string) {
        Data += String(string);
        return *this;
    }

    StringStream& operator <<(unsigned long string) {
        Data += String(string);
        return *this;
    }

    StringStream& operator <<(float string) {
        Data += String(string);
        return *this;
    }

    StringStream& operator <<(double string) {
        Data += String(string);
        return *this;
    }

    operator String() const {
        return Data;
    }

    void clear() {
        Data.clear();
    }

    String::size_type size() const {
        return Data.size();
    }

    void reserve(String::size_type size) {
        Data.reserve(size);
    }

    String& str() {
        return Data;
    }

protected:
    String Data;
};

//class StringChain {
//public:
//    StringChain()
//        : Head(nullptr)
//    {}
//
//    StringChain& operator <<(StringBuf string) {
//        if (Head == nullptr) {
//            Head = new TStringLink(string);
//        } else {
//
//        }
//    }
//
//protected:
//    struct TStringLink {
//        String Data;
//        TStringLink* Next;
//
//        TStringLink(const String& data)
//            : Data(data)
//            , Next(nullptr)
//        {}
//
//        TStringLink(String&& data)
//            : Data(data)
//            , Next(nullptr)
//        {}
//    };
//
//    TStringLink* Head;
//};

/*class StringArray {
public:
    using size_type = String::size_type;

    StringArray()
        : Size(0)
    {}

    StringArray& operator <<(StringBuf string) {
        AddString(string);
        return *this;
    }

    StringArray& operator <<(char string) {
        AddString(String(&string, 1));
        return *this;
    }

    StringArray& operator <<(int string) {
        AddString(String(string));
        return *this;
    }

    StringArray& operator <<(unsigned int string) {
        AddString(String(string));
        return *this;
    }

    StringArray& operator <<(long string) {
        AddString(String(string));
        return *this;
    }

    StringArray& operator <<(unsigned long string) {
        AddString(String(string));
        return *this;
    }

    StringArray& operator <<(float string) {
        AddString(String(string));
        return *this;
    }

    StringArray& operator <<(double string) {
        AddString(String(string));
        return *this;
    }

    size_type length() const {
        return GetFirstString().length();
    }

    size_type total_size() const {
        size_type size = 0;
        for (uint8_t i = 0; i < Size; ++i) {
            size += Array[i].size();
        }
        return size;
    }

    const char* begin() const {
        return GetFirstString().begin();
    }

    void erase(size_type pos, size_type length) {
        GetFirstString().erase(pos, length);
    }

protected:
    void AddString(const String& data) {
        if (Size < 10) {
            Array[Size++] = data;
        } else {
            Reset();
        }
    }

    String& GetFirstString() {
        for (uint8_t i = 0; i < Size; ++i) {
            if (!Array[i].empty()) {
                return Array[i];
            }
        }
        static String Default;
        return Default;
    }

    const String& GetFirstString() const {
        return const_cast<StringArray*>(this)->GetFirstString();
    }

    String Array[10] = {};
    size_type Size;
};*/

}
