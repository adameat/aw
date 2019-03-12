#pragma once

namespace AW {

class TTime {
public:
    constexpr TTime()
        : Value() {
    }

    constexpr bool operator ==(TTime time) const { return Value == time.Value; }
    constexpr bool operator !=(TTime time) const { return Value != time.Value; }
    constexpr bool operator <(TTime time) const { return Value < time.Value; }
    constexpr bool operator <=(TTime time) const { return Value <= time.Value; }
    constexpr bool operator >(TTime time) const { return Value > time.Value; }
    constexpr bool operator >=(TTime time) const { return Value >= time.Value; }
    constexpr TTime operator +(TTime time) const { return TTime(Value + time.Value); }
    constexpr TTime operator -(TTime time) const { return TTime(Value - time.Value); }
    TTime& operator +=(TTime time) { Value += time.Value; return *this; }
    TTime& operator -=(TTime time) { Value -= time.Value; return *this; }
    constexpr TTime operator *(int value) const { return TTime(Value * value); }
    static constexpr TTime MilliSeconds(unsigned long ms) { return TTime(ms); }
    static constexpr TTime Seconds(unsigned long s) { return MilliSeconds(s * 1000); }
    static constexpr TTime Minutes(unsigned long s) { return Seconds(s * 60); }
    static constexpr TTime Hours(unsigned long s) { return Minutes(s * 60); }
    static TTime Now() { return TTime(millis()); }
    String AsString() const;
    constexpr unsigned long MilliSeconds() const { return Value; }
    constexpr unsigned long Seconds() const { return Value / 1000; }
    constexpr unsigned long Minutes() const { return Value / (1000L * 60); }
    constexpr unsigned long Hours() const { return Value / (1000L * 60 * 60); }
    static constexpr TTime Max() { return TTime((unsigned long)-1); }
    static constexpr TTime Zero() { return TTime(0); }
    constexpr bool IsValid() const { return Value != 0; }

protected:
    constexpr TTime(unsigned long ms)
        : Value(ms) {
    }

    unsigned long Value;
};

}