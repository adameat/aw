#pragma once

namespace AW {

template <typename Type, int WindowSize = 60>
class TAverage {
public:
    TAverage()
        : Accumulator()
        , Count() {
    }

    void AddValue(Type value) {
        if (Count >= WindowSize) {
            Accumulator /= 2;
            Count /= 2;
        }
        Accumulator += value;
        Count += 1;
    }

    void SetValue(Type value, int count = 1) {
        Accumulator = value;
        Count = count;
    }

    void SetValue(StringBuf value) {
        StringBuf accumulator = value.NextToken('/');
        Accumulator = accumulator;
        Count = value.empty() ? 1 : value;
    }

    Type GetValue() const {
        return Accumulator / Count;
    }

    Type GetSum() const {
        return Accumulator;
    }

    int GetCount() const {
        return Count;
    }

    void Clear() {
        Accumulator = Type();
        Count = 0;
    }

    void Reset() {
        SetValue(GetValue());
    }

    bool IsValid(int samples = 1) const {
        return GetCount() >= samples;
    }

    void operator =(Type value) {
        AddValue(value);
    }

    operator Type() const {
        return GetValue();
    }

    String ToString() const {
        return StringStream() << Accumulator << '/' << Count;
    }

protected:
    Type Accumulator;
    int Count;
};

template <typename Type>
class TAverage<Type, 0> {
public:
    TAverage()
        : Accumulator() {
    }

    void AddValue(Type value) {
        Accumulator = value;
    }

    Type GetValue() const {
        return Accumulator;
    }

    Type GetSum() const {
        return Accumulator;
    }

    static constexpr int GetCount() {
        return 1;
    }

    void Clear() {
        Accumulator = Type();
    }

    void Reset() {}

    bool IsValid() const {
        return Accumulator == Type();
    }

    void operator =(Type value) {
        AddValue(value);
    }

    operator Type() const {
        return GetValue();
    }

protected:
    Type Accumulator;
};


}
