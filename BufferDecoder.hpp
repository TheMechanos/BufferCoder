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
        static_assert(std::is_integral<T>::value, "T must be an integral type");

        if (!hasSpaceInBuffer(pos, sizeof(T))) return 0;

        value = decodeIntegralTemplate<T>(pos);
        return sizeof(T);
    }

    size_t decodeU32Packed(size_t pos, uint32_t& data) {
        uint32_t value = 0;

        for (size_t size = 0; size < 4; size++) {
            if (!hasSpaceInBuffer(pos, size + 1)) return 0;

            uint8_t byte = buffer[pos + size];
            value |= static_cast<uint32_t>(byte & 0x7F) << (7 * size);

            if ((byte & 0x80) == 0) {
                data = value;
                return size + 1;
            }
        }

        // Continuation bit set on the 4th byte - malformed data
        return 0;
    }

    // Returns dataSize - for dataSize == 0 the result is 0 (nothing read, not an error)
    size_t decodeBuffer(size_t pos, void* data, size_t dataSize) {
        if (!hasSpaceInBuffer(pos, dataSize)) return 0;

        if (dataSize > 0) memcpy(data, &buffer[pos], dataSize);
        return dataSize;
    }

    // maxSize - capacity of the destination buffer, dataSize - receives decoded length
    size_t decodeBufferWithLength(size_t pos, void* data, size_t maxSize, size_t& dataSize) {
        if (!hasSpaceInBuffer(pos, 2)) return 0;

        uint16_t len = 0;
        decodeIntegral(pos, len);

        // Length comes from untrusted data - validate against destination and source sizes
        if (len > maxSize) return 0;
        if (!hasSpaceInBuffer(pos + 2, len)) return 0;

        if (len > 0) memcpy(data, &buffer[pos + 2], len);
        dataSize = len;

        return 2 + len;
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
        : BufferDecoder(buffer, size, endianness)
        , pos(0)
        , anyErrors(false) { }


    bool skipBytes(size_t bytes) {
        for (size_t q = 0; q < bytes; q++) {
            bool e;
            if (!decodeBool(e)) return false;
        }
        return true;
    }

    bool decodeBool(bool& data) {
        size_t read = BufferDecoder::decodeBool(pos, data);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    template <typename T> bool decodeIntegral(T& value) {
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
        // Empty buffer is a valid no-op
        if (dataSize == 0) return true;

        size_t read = BufferDecoder::decodeBuffer(pos, data, dataSize);
        pos += read;
        if (read == 0) anyErrors = true;
        return read > 0;
    }

    bool decodeBufferWithLength(void* data, size_t maxSize, size_t& dataSize) {
        size_t read = BufferDecoder::decodeBufferWithLength(pos, data, maxSize, dataSize);
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

    // Generic decode - dispatches to BufferConverter<T> (include BufferCoder.hpp)
    template <typename T> bool decode(T& value) {
        return BufferConverter<typename std::remove_cv<T>::type>::decode(*this, value);
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