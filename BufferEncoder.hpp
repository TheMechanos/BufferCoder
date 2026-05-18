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

    template <typename T> T encodeIntegral(size_t pos, T value) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        if (!hasSpaceInBuffer(pos, sizeof(T))) return 0;

        BufferCoderCommon::encodeIntegral(pos, value);
        return sizeof(T);
    }


    size_t encodeU32Packed(size_t pos, uint32_t data) {
        uint8_t byte;
        size_t size = 0;

        if (data == 0) {
            buffer[pos] = 0x00;
            return 1;
        }

        while (data > 0) {
            if (!hasSpaceInBuffer(pos, size + 1)) return 0;

            byte = data & 0x7F;
            data >>= 7;
            if (data > 0) byte |= 0x80;

            buffer[pos + size] = byte;
            size++;
        }
        return size;
    }

    size_t encodeBuffer(size_t pos, void* data, size_t dataSize) {
        if (!hasSpaceInBuffer(pos, dataSize)) return 0;

        memcpy(&buffer[pos], data, dataSize);
        return dataSize;
    }

    size_t encodeBufferWithLength(size_t pos, void* data, size_t dataSize) {
        if (!hasSpaceInBuffer(pos, dataSize + 2)) return 0;

        encodeIntegral(pos, static_cast<uint16_t>(dataSize));
        return encodeBuffer(pos + 2, data, dataSize);
    }

    size_t encodeString(size_t pos, char* data) {
        size_t len = strlen(data);

        if (!hasSpaceInBuffer(pos, len + 1)) return 0;

        memcpy(&buffer[pos], data, len);
        buffer[pos+len] = 0x00;
        return len + 1;
    }

};






class BufferEncoderCursor : public BufferEncoder {
public:
    BufferEncoderCursor(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : BufferEncoder(buffer, size, endianness), pos(0) { }

    bool encodeBool(bool data) {
        size_t written = BufferEncoder::encodeBool(pos, data);
        pos += written;
        return written > 0;
    }

    template <typename T>
    bool encodeIntegral(T value) {
        size_t written = BufferEncoder::encodeIntegral(pos, value);
        pos += written;
        return written > 0;
    }

    bool encodeU32Packed(uint32_t data) {
        size_t written = BufferEncoder::encodeU32Packed(pos, data);
        pos += written;
        return written > 0;
    }

    bool encodeBuffer(void* data, size_t dataSize) {
        size_t written = BufferEncoder::encodeBuffer(pos, data, dataSize);
        pos += written;
        return written > 0;
    }

    bool encodeBufferWithLength(void* data, size_t dataSize) {
        size_t written = BufferEncoder::encodeBufferWithLength(pos, data, dataSize);
        pos += written;
        return written > 0;
    }

    bool encodeString(char* data) {
        size_t written = BufferEncoder::encodeString(pos, data);
        pos += written;
        return written > 0;
    }

    size_t getPos() const { return pos; }
    void resetPos() { pos = 0; }
    void setPos(size_t newPos) { pos = newPos; }

private:
    size_t pos;
};