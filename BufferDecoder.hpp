#pragma once

#include <BufferCoderCommon.hpp>



class BufferDecoder : public BufferCoderCommon {
public:
    BufferDecoder(const void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : BufferCoderCommon(const_cast<void*>(buffer), size, endianness) { }

    size_t decodeBool(size_t pos, bool& data) {
        if (!hasSpaceInBuffer(pos, 1)) return 0;
        data = buffer[pos] != 0x00;
        return 1;
    }

    template <typename T> size_t decodeIntegral(size_t pos, T& value) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        if (!hasSpaceInBuffer(pos, sizeof(T))) return 0;

        decodeIntegralTemplate(pos, value);
        return sizeof(T);
    }

    size_t decodeU32Packed(size_t pos, uint32_t& data) {
        uint32_t value = 0;
        uint32_t shift = 1;
        size_t size = 0;

        while (size < 4) {
            if (!hasSpaceInBuffer(pos, size + 1)) return 0;

            uint8_t byte = buffer[pos + size];
            value += (byte & 0x7F) * shift;
            size++;

            if (byte < 0x80) break;

            shift *= 0x80;
        }

        data = value;
        return size;
    }

    size_t decodeBuffer(size_t pos, void* data, size_t dataSize) {
        if (!hasSpaceInBuffer(pos, dataSize)) return 0;

        memcpy(data, &buffer[pos], dataSize);
        return dataSize;
    }

    size_t decodeBufferWithLength(size_t pos, void* data, size_t& dataSize) {
        if (!hasSpaceInBuffer(pos, 2)) return 0;

        uint16_t len = 0;
        decodeIntegral(pos, len);
        dataSize = len;

        return 2 + decodeBuffer(pos + 2, data, dataSize);
    }

    size_t decodeString(size_t pos, char* data, size_t maxLen) {
        size_t i = 0;

        while (i < maxLen) {
            if (!hasSpaceInBuffer(pos, i + 1)) return 0;

            data[i] = static_cast<char>(buffer[pos + i]);
            if (data[i] == 0x00) return i + 1;
            i++;
        }

        return 0;
    }
};



class BufferDecoderCursor : public BufferDecoder {
public:
    BufferDecoderCursor(const void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian)
        : BufferDecoder(buffer, size, endianness), pos(0), anyErrors(false) { }

    bool decodeBool(bool& data) {
        size_t read = BufferDecoder::decodeBool(pos, data);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    template <typename T>
    bool decodeIntegral(T& value) {
        size_t read = BufferDecoder::decodeIntegral(pos, value);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    bool decodeU32Packed(uint32_t& data) {
        size_t read = BufferDecoder::decodeU32Packed(pos, data);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    bool decodeBuffer(void* data, size_t dataSize) {
        size_t read = BufferDecoder::decodeBuffer(pos, data, dataSize);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    bool decodeBufferWithLength(void* data, size_t& dataSize) {
        size_t read = BufferDecoder::decodeBufferWithLength(pos, data, dataSize);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    bool decodeString(char* data, size_t maxLen) {
        size_t read = BufferDecoder::decodeString(pos, data, maxLen);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
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