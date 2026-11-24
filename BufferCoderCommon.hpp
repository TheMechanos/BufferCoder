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


    size_t getBufferSize() const { return maxSize; }

protected:
    // Written to avoid size_t overflow of (pos + size) for large pos values
    bool hasSpaceInBuffer(size_t pos, size_t size) const { return pos <= maxSize && size <= maxSize - pos; }

    // Bit operations are done on uint64_t to avoid UB when shifting signed values
    template <typename T> inline void encodeIntegralTemplate(size_t pos, T data) {
        static_assert(TYPE_TRAIT_VALUE(is_integral, T), "T must be an integral type");
        static_assert(sizeof(T) <= sizeof(uint64_t), "T must be at most 64 bits wide");

        const size_t size = sizeof(T);
        const uint64_t raw = static_cast<uint64_t>(data);

        for (size_t i = 0; i < size; ++i) {
            size_t shift = (endianness == Endianness::LittleEndian) ? (8 * i) : (8 * (size - 1 - i));
            buffer[pos + i] = static_cast<uint8_t>((raw >> shift) & 0xFF);
        }
    }

    template <typename T> inline T decodeIntegralTemplate(size_t pos) {
        static_assert(TYPE_TRAIT_VALUE(is_integral, T), "T must be an integral type");
        static_assert(sizeof(T) <= sizeof(uint64_t), "T must be at most 64 bits wide");

        const size_t size = sizeof(T);
        uint64_t raw = 0;

        for (size_t i = 0; i < size; ++i) {
            size_t shift = (endianness == Endianness::LittleEndian) ? (8 * i) : (8 * (size - 1 - i));
            raw |= static_cast<uint64_t>(buffer[pos + i]) << shift;
        }

        return static_cast<T>(raw);
    }



    uint8_t* buffer;
    size_t maxSize;
    Endianness endianness;
};