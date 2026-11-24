#pragma once

#include <BufferCoderCommon.hpp>


class BufferEncoder : public BufferCoderCommon {
public:
    BufferEncoder(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : BufferCoderCommon(buffer, size, endianness) { };


    size_t encodeBool(size_t pos, bool data) {
        if (!hasSpaceInBuffer(pos, 1)) return 0;
        buffer[pos] = data ? 0x01 : 0x00;
        return 1;
    }

    template <typename T> size_t encodeIntegral(size_t pos, T value) {
        static_assert(std::is_integral<T>::value, "T must be an integral type");

        if (!hasSpaceInBuffer(pos, sizeof(T))) return 0;

        encodeIntegralTemplate(pos, value);
        return sizeof(T);
    }


    size_t encodeU32Packed(size_t pos, uint32_t data) {
        // Max 4 bytes x 7 bits - larger values cannot be decoded
        if (data > U32PackedMax) return 0;

        size_t size = 0;

        // do-while so that value 0 is also encoded (as a single 0x00 byte) with bounds check
        do {
            if (!hasSpaceInBuffer(pos, size + 1)) return 0;

            uint8_t byte = data & 0x7F;
            data >>= 7;
            if (data > 0) byte |= 0x80;

            buffer[pos + size] = byte;
            size++;
        } while (data > 0);

        return size;
    }

    // Returns dataSize - for dataSize == 0 the result is 0 (nothing written, not an error)
    size_t encodeBuffer(size_t pos, const void* data, size_t dataSize) {
        if (!hasSpaceInBuffer(pos, dataSize)) return 0;

        if (dataSize > 0) memcpy(&buffer[pos], data, dataSize);
        return dataSize;
    }

    size_t encodeBufferWithLength(size_t pos, const void* data, size_t dataSize) {
        // Length is stored as uint16_t - larger payloads cannot be represented
        if (dataSize > UINT16_MAX) return 0;
        if (!hasSpaceInBuffer(pos, dataSize + 2)) return 0;

        encodeIntegral(pos, static_cast<uint16_t>(dataSize));
        if (dataSize > 0) memcpy(&buffer[pos + 2], data, dataSize);
        return 2 + dataSize;
    }

    size_t encodeString(size_t pos, const char* data) {
        size_t len = strlen(data);

        if (!hasSpaceInBuffer(pos, len + 1)) return 0;

        memcpy(&buffer[pos], data, len);
        buffer[pos + len] = 0x00;
        return len + 1;
    }
};





class BufferEncoderCursor : public BufferEncoder {
public:
    BufferEncoderCursor(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : BufferEncoder(buffer, size, endianness)
        , pos(0)
        , anyErrors(false) { }


    bool skipBytes(size_t bytes) {
        for (size_t q = 0; q < bytes; q++) {
            if (!encodeBool(false)) return false;
        }
        return true;
    }

    bool encodeBool(bool data) {
        size_t written = BufferEncoder::encodeBool(pos, data);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    template <typename T> bool encodeIntegral(T value) {
        size_t written = BufferEncoder::encodeIntegral(pos, value);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    bool encodeU32Packed(uint32_t data) {
        size_t written = BufferEncoder::encodeU32Packed(pos, data);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    bool encodeBuffer(const void* data, size_t dataSize) {
        // Empty buffer is a valid no-op
        if (dataSize == 0) return true;

        size_t written = BufferEncoder::encodeBuffer(pos, data, dataSize);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    bool encodeBufferWithLength(const void* data, size_t dataSize) {
        size_t written = BufferEncoder::encodeBufferWithLength(pos, data, dataSize);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    bool encodeString(const char* data) {
        size_t written = BufferEncoder::encodeString(pos, data);
        pos += written;
        if (written == 0) anyErrors = true;
        return written > 0;
    }

    // Generic encode - dispatches to BufferConverter<T> (include BufferCoder.hpp)
    template <typename T> bool encode(const T& value) {
        return BufferConverter<typename std::remove_cv<T>::type>::encode(*this, value);
    }

    size_t getPos() const { return pos; }
    void setPos(size_t newPos) { pos = newPos; }
    void resetPos() { pos = 0; }

    bool hasAnyErrors() const { return anyErrors; }
    void clearErrors() { anyErrors = false; }

private:
    size_t pos;
    bool anyErrors;
};