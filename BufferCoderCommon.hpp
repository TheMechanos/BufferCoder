#pragma once

#include <stdint.h>


class BufferCoderCommon {
public:
    enum class Endianness : uint8_t { BigEndian, LittleEndian };

    BufferCoderCommon(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : buffer(static_cast<uint8_t*>(buffer))
        , maxSize(size)
        , endianness(endianness) { }


    size_t getBufferSize() { return maxSize; }

protected:
    bool hasSpaceInBuffer(size_t pos, size_t size) { return pos + size <= maxSize; }

    template <typename T> inline void encodeIntegral(size_t pos, T data) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        const size_t size = sizeof(T);

        for (size_t i = 0; i < size; ++i) {
            size_t shift = (endianness == Endianness::LittleEndian) ? (8 * i) : (8 * (size - 1 - i));
            buffer[pos + i] = static_cast<uint8_t>((data >> shift) & 0xFF);
        }
    }

    template <typename T> inline T decodeIntegral(size_t pos) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        const size_t size = sizeof(T);
        T value = 0;

        for (size_t i = 0; i < size; ++i) {
            size_t shift = (endianness == Endianness::LittleEndian) ? (8 * i) : (8 * (size - 1 - i));

            value |= (static_cast<T>(buffer[pos + i]) << shift);
        }

        return value;
    }



    uint8_t* buffer;
    size_t maxSize;
    Endianness endianness;
};