#pragma once

template <int points>
struct TFixedPointConstants {
};

template <>
struct TFixedPointConstants<3> {
    static constexpr int32_t MULTIPLIER = 1000;
    static constexpr int32_t NOT_A_NUMBER = 0x10000000;
    struct NOT_A_NUMBER_MARKER {};
};

template <int points = 3>
struct TFixedPointValue {
    static constexpr TFixedPointValue NOT_A_NUMBER = { TFixedPointConstants<points>::NOT_A_NUMBER_MARKER() };
    int32_t Value;

    bool operator ==(TFixedPointValue<points> other) const {
        return Value == other.Value;
    }

    bool operator !=(TFixedPointValue<points> other) const {
        return Value != other.Value;
    }

    TFixedPointValue operator /(TFixedPointValue value) const {
        TFixedPointValue n;
        n.Value = Value * TFixedPointConstants<points>::MULTIPLIER / value.Value;
        return n;
    }

    TFixedPointValue& operator /=(TFixedPointValue value) {
        Value = Value * TFixedPointConstants<points>::MULTIPLIER / value.Value;
        return *this;
    }

    TFixedPointValue operator +(TFixedPointValue value) const {
        TFixedPointValue n;
        n.Value = Value + value.Value;
        return n;
    }

    TFixedPointValue& operator +=(TFixedPointValue value) {
        Value += value.Value;
        return *this;
    }

    TFixedPointValue()
        : Value()
    {}

    TFixedPointValue(unsigned long other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    TFixedPointValue(int other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    TFixedPointValue(int32_t other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    TFixedPointValue(int64_t other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    constexpr TFixedPointValue(float other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    constexpr TFixedPointValue(double other)
        : Value(other * TFixedPointConstants<points>::MULTIPLIER)
    {}

    TFixedPointValue& operator =(unsigned long other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    TFixedPointValue& operator =(int other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    TFixedPointValue& operator =(int32_t other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    TFixedPointValue& operator =(int64_t other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    TFixedPointValue& operator =(float other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    TFixedPointValue& operator =(double other) {
        Value = other * TFixedPointConstants<points>::MULTIPLIER;
        return *this;
    }

    bool isnan() const {
        return Value == TFixedPointConstants<points>::NOT_A_NUMBER;
    }

    void setnan() {
        Value = TFixedPointConstants<points>::NOT_A_NUMBER;
    }

    int32_t raw() const {
        return Value;
    }

    void raw(int32_t v) {
        Value = v;
    }
};

using fixed3_t = TFixedPointValue<3>;
