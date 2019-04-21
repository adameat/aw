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
    using size_type = unsigned int;
    static constexpr size_type npos = ~(size_type)0;

    StringPointer(const char* ptr);

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

    template <int N>
    constexpr StringBuf(const char(&string)[N])
        : StringBuf(&string[0], &string[N - 1])
    {}

    StringBuf(StringPointer ptr)
        : Begin(ptr.begin)
        , End(ptr.begin + ptr.length)
    {}

    StringBuf(const char* ptr);

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

    bool operator ==(const StringBuf& s) const;

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

    StringBuf substr(size_type pos, size_type spos = npos) const;
    bool starts_with(const StringBuf& s) const;
    size_type find(char c) const;
    unsigned char touchar() const;
    int toint() const;
    unsigned long toulong() const;
    long tolong() const;
    double todouble() const;
    float tofloat() const;

    char operator [](size_type pos) const {
        return *(Begin + pos);
    }

    operator unsigned char() const {
        return touchar();
    }

    operator int() const {
        return toint();
    }

	operator unsigned long() const {
		return toulong();
	}

	operator long() const {
		return tolong();
	}

	operator double() const {
		return todouble();
	}

    operator float() const {
        return tofloat();
    }

    StringBuf NextToken(char delimeter = ' ');
    uint16_t crc16() const;

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

    String(const char* ptr);

    static char ConversionBuffer[32];

    String(StringPointer ptr)
        : String(ptr.begin, ptr.length)
    {}

    String(StringBuf buf)
        : String(buf.begin(), buf.size())
    {}

    String(unsigned int value, unsigned char base = 10);
    String(int value, unsigned char base = 10);
    String(unsigned long value, unsigned char base = 10);
    String(long value, unsigned char base = 10);
    String(float value, unsigned char decimalPlaces = 2);
    String(double value, unsigned char decimalPlaces = 2);
    String(const String& string);
    String(String&& string);

    String& operator =(const String& string);
    String& operator =(String&& string);

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

    void assign(const char* string, size_type length);
    void append(const char* string, size_type length);
    void erase(size_type pos, size_type length);
    String substr(size_type pos, size_type length = npos) const;
    void reserve(size_type length);
    void resize(size_type length);

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

    size_type capacity() const;
    bool check() const;
    bool _IsShared() const { return Buffer == nullptr || Buffer->RefCounter > 1; }
    bool _IsUnique() const { return Buffer != nullptr && Buffer->RefCounter == 1; }

protected:
    void EnsureOneOwner(size_type size = 0);

    String(const String& string, size_type pos, size_type length);

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

    void Alloc(size_type length);
    void Free();

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
