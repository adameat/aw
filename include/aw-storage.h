#pragma once
#ifdef ARDUINO_ARCH_AVR
//#include <avr/eeprom.h>
#endif

namespace AW {

#ifdef ARDUINO_ARCH_AVR
class TArduinoEEPROM {
public:
    static constexpr uint16_t length() {
        return E2END + 1;
    }

    static uint8_t read(uint16_t offset) {
        return eeprom_read_byte(reinterpret_cast<const uint8_t*>(offset));
    }

    static void write(uint16_t offset, uint8_t byte) {
        eeprom_write_byte(reinterpret_cast<uint8_t*>(offset), byte);
    }
};
#endif

}
