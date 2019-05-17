#pragma once

namespace AW {

template <typename TByteArray>
class TFileSystem {
protected:
    TByteArray& ByteArray;

    struct THeader {
        uint8_t NameLength;
        char Name[];
        uint8_t DataLength;
    };

    static constexpr uint8_t MINIMUM_HEADER_SIZE = 2;
    static constexpr uint8_t MAXIMUM_DATA_SIZE = 255;

    class iterator {
    public:
        iterator(TByteArray& array, size_t offset)
            : Array(array)
            , Offset(offset)
        {}

        String GetString(size_t offset, uint8_t length) {
            String name;
            name.reserve(length);
            while (length-- > 0) {
                name += Array.read(offset++);
            }
            return name;
        }

        void PutString(size_t offset, StringBuf string) {
            for (char c : string) {
                Array.write(offset++, c);
            }
        }

        String GetName() {
            uint8_t nameLength = Array.read(Offset);
            if (nameLength == 0) {
                return String();
            }
            return GetString(Offset + sizeof(nameLength), nameLength);
        }

        String GetData() {
            uint8_t nameLength = Array.read(Offset);
            uint8_t dataLength = Array.read(Offset + sizeof(nameLength) + nameLength);
            return GetString(Offset + sizeof(nameLength) + nameLength + sizeof(dataLength), dataLength);
        }

        uint8_t GetDataSize() {
            uint8_t nameLength = Array.read(Offset);
            uint8_t dataLength = Array.read(Offset + sizeof(nameLength) + nameLength);
            return dataLength;
        }

        void SetDataSize(uint8_t dataSize) {
            uint8_t nameLength = Array.read(Offset);
            Array.write(Offset + sizeof(nameLength) + nameLength, dataSize);
        }

        iterator& operator ++() {
            uint8_t nameLength = Array.read(Offset);
            Offset += sizeof(nameLength);
            Offset += nameLength;
            uint8_t dataLength = Array.read(Offset);
            Offset += sizeof(dataLength);
            Offset += dataLength;
            return *this;
        }

        void PutData(StringBuf data) {
            auto offset = Offset;
            uint8_t nameLength = Array.read(offset);
            offset += sizeof(nameLength);
            offset += nameLength;
            uint8_t dataLength = (uint8_t)data.size();
            Array.write(offset++, dataLength);
            for (String::size_type pos = 0; pos < dataLength; ++pos) {
                Array.write(offset++, data[pos]);
            }
        }

        bool Put(StringBuf name, StringBuf data) {
            uint8_t size = GetDataSize();
            uint8_t nameLength = (uint8_t)name.size();
            uint8_t dataLength = (uint8_t)data.size();
            if (IsFree() && nameLength + dataLength + MINIMUM_HEADER_SIZE <= size) {
                auto offset = Offset;
                Array.write(offset++, nameLength);
                PutString(offset, name);
                offset += nameLength;
                Array.write(offset++, dataLength);
                PutString(offset, data);
                offset += dataLength;
                Array.write(offset++, 0); // free
                Array.write(offset++, size - nameLength - dataLength - MINIMUM_HEADER_SIZE);
                return true;
            }
            return false;
        }

        bool IsFree() {
            uint8_t nameLength = Array.read(Offset);
            return nameLength == 0;
        }

        bool IsSystem() {
            uint8_t nameLength = Array.read(Offset);
            return nameLength >= 0 && Array.read(Offset + 1) == '$';
        }

        bool IsEnoughSize(uint16_t requiredSize) {
            auto dataSize = GetDataSize();
            return dataSize == requiredSize || dataSize >= requiredSize + MINIMUM_HEADER_SIZE;
        }

        void Erase() {
            auto offset = Offset;
            uint8_t nameLength = Array.read(offset);
            uint8_t dataLength = Array.read(offset + sizeof(nameLength) + nameLength);
            Array.write(offset++, 0);
            Array.write(offset, nameLength + dataLength);
        }

        bool operator ==(const iterator& it) const {
            return Offset == it.Offset;
        }

        bool operator !=(const iterator& it) const {
            return Offset != it.Offset;
        }

        bool operator <(const iterator& it) const {
            return Offset < it.Offset;
        }

        void operator =(const iterator& it) {
            Offset = it.Offset;
        }

    protected:
        TByteArray& Array;
        size_t Offset;
    };

public:
    TFileSystem(TByteArray& array)
        : ByteArray(array)
    {
        auto it = begin();
        if (it.GetName() != "$HDR" || !it.GetData().starts_with("AW")) {
            Format();
        }
    }

    iterator begin() {
        return iterator(ByteArray, 0);
    }

    iterator end() {
        return iterator(ByteArray, ByteArray.length());
    }

    bool WriteFile(StringBuf name, StringBuf data) {
        auto nameSize = name.size();
        auto dataSize = data.size();
        if (nameSize > MAXIMUM_DATA_SIZE) {
            return false;
        }
        if (dataSize > MAXIMUM_DATA_SIZE) {
            return false;
        }
        uint16_t requiredSize = (uint8_t)nameSize + (uint8_t)dataSize + MINIMUM_HEADER_SIZE;
        if (requiredSize > MAXIMUM_DATA_SIZE) {
            return false;
        }
        for (iterator it = begin(); it < end();) {
            if (it.GetName() == name) {
                if (it.GetDataSize() == dataSize) {
                    it.PutData(data);
                    return true;
                } else {
                    it.Erase();
                    Defrag();
                    break;
                }
            }
            ++it;
        }
        for (iterator it = begin(); it < end();) {
            if (it.IsFree() && it.IsEnoughSize(requiredSize)) {
                bool result = it.Put(name, data);
                if (result) {
                    Defrag();
                }
                return result;
            }
            ++it;
        }
        return false;
    }

    bool ReadFile(StringBuf name, String& data) {
        for (auto it = begin(); it < end(); ++it) {
            if (it.GetName() == name) { // TODO: optimize
                data = it.GetData();
                return true;
            }
        }
        return false;
    }

    bool EraseFile(StringBuf name) {
        for (iterator it = begin(); it < end();) {
            if (it.GetName() == name) {
                it.Erase();
                Defrag();
                return true;
            }
            ++it;
        }
        return false;
    }

    template <typename Type>
    bool WriteValue(StringBuf name, Type value) {
        return WriteFile(name, String(value));
    }

    template <typename Type>
    Type ReadValue(StringBuf name, Type defaultValue = Type()) {
        String value;
        if (ReadFile(name, value)) {
            return value;
        } else {
            return defaultValue;
        }
    }

    void Defrag() {
        for (auto current = begin(); current < end(); ++current) {
            if (current.IsFree()) {
                auto next = current;
                ++next;
                if (next < end() && next.IsFree()) {
                    uint16_t dataSize1 = current.GetDataSize();
                    uint16_t dataSize2 = next.GetDataSize();
                    uint16_t requiredSize = dataSize1 + dataSize2 + MINIMUM_HEADER_SIZE;
                    if (requiredSize <= MAXIMUM_DATA_SIZE) {
                        current.SetDataSize((uint8_t)requiredSize);
                    }
                }
            }
        }
    }

    bool Check() {
        auto it = begin();
        while (it < end()) {
            ++it;
        }
        return it == end();
    }

    void Format() {
        size_t size = ByteArray.length();
        size_t offset = 0;
        while (size > 0) {
            uint8_t block = (uint8_t)min(size - (size_t)MINIMUM_HEADER_SIZE, (size_t)MAXIMUM_DATA_SIZE);
            ByteArray.write(offset++, 0);
            ByteArray.write(offset++, block);
            offset += block;
            size -= 2;
            size -= block;
        }
        WriteFile("$HDR", "AW1");
    }
};

}
