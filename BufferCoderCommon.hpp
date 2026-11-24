#pragma once

#include <stdint.h>
#include <string.h>

#include <type_traits>

#if defined(__cpp_variable_templates) && (__cpp_variable_templates >= 201304)
    #define TYPE_TRAIT_VALUE(trait, type) std::trait##_v<type>
#else
    #define TYPE_TRAIT_VALUE(trait, type) std::trait<type>::value
#endif

class BufferCoderCommon {
public:
    enum class Endianness : uint8_t { BigEndian, LittleEndian };

    // Largest value representable by U32Packed encoding (4 bytes x 7 bits)
    static constexpr uint32_t U32PackedMax = 0x0FFFFFFF;

    BufferCoderCommon(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : buffer(static_cast<uint8_t*>(buffer))
        , maxSize(size)
        , endianness(endianness) { }


    size_t getBufferSize() { return maxSize; }

protected:
    bool hasSpaceInBuffer(size_t pos, size_t size) { return pos + size <= maxSize; }

    template <typename T> inline void encodeIntegralTemplate(size_t pos, T data) {
        static_assert(TYPE_TRAIT_VALUE(is_integral, T), "T must be an integral type");

        const size_t size = sizeof(T);

        for (size_t i = 0; i < size; ++i) {
            size_t shift = (endianness == Endianness::LittleEndian) ? (8 * i) : (8 * (size - 1 - i));
            buffer[pos + i] = static_cast<uint8_t>((data >> shift) & 0xFF);
        }
    }

    template <typename T> inline T decodeIntegralTemplate(size_t pos) {
        static_assert(TYPE_TRAIT_VALUE(is_integral, T), "T must be an integral type");

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