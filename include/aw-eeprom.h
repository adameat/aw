#include <avr/eeprom.h>

namespace AW {

template <typename StructType>
class TEEPROM {
public:
    template <typename ValueType>
    ValueType Get(ValueType StructType::* ptr) const {
        int idx = static_cast<int>(reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0));
        ValueType value;
        Get(idx, value);
        return value;
    }

    template <typename ValueType>
    void Put(ValueType StructType::* ptr, ValueType val) const {
        int idx = static_cast<int>(reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0));
        Put(idx, val);
    }

    template <typename ValueType>
    void Update(ValueType StructType::* ptr, ValueType val) const {
        int idx = static_cast<int>(reinterpret_cast<uint8_t*>(&(static_cast<StructType*>(0)->*ptr)) - reinterpret_cast<uint8_t*>(0));
        Update(idx, val);
    }

    void Put(const StructType& val) {
        Put(0, val);
    }

    void Update(const StructType& val) {
        Update(0, val);
    }

protected:
    template <typename T>
    static void Get(int idx, T& val) {
        for (size_t i = 0; i < sizeof(T); ++i) {
            ((uint8_t*)&val)[i] = eeprom_read_byte(reinterpret_cast<uint8_t*>(idx + i));
        }
    }

    template <typename T>
    static void Put(int idx, const T& val) {
        for (size_t i = 0; i < sizeof(T); ++i) {
            eeprom_write_byte(reinterpret_cast<uint8_t*>(idx + i), ((uint8_t*)&val)[i]);
        }
    }

    template <typename T>
    static void Update(int idx, const T& val) {
        for (size_t i = 0; i < sizeof(T); ++i) {
            if (eeprom_read_byte(reinterpret_cast<uint8_t*>(idx + i)) != ((uint8_t*)&val)[i]) {
                eeprom_write_byte(reinterpret_cast<uint8_t*>(idx + i), ((uint8_t*)&val)[i]);
            }
        }
    }
};

}