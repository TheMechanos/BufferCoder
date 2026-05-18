# BufferCoder

Lightweight C++ library for binary serialization and deserialization into/from raw memory buffers. Designed for embedded systems and network protocols where precise control over byte layout is required.

---

## Class hierarchy

```
BufferCoderCommon
├── BufferEncoder
│   └── BufferEncoderCursor
└── BufferDecoder
    └── BufferDecoderCursor
```

`BufferCoderCommon` holds the buffer pointer, size, and endianness. Encoder and decoder inherit from it and add their respective read/write methods. The `Cursor` variants track the current position automatically.

---

## Quick start

```cpp
uint8_t buf[64];

// Encoder with manual position
BufferEncoder enc(buf, sizeof(buf), Endianness::LittleEndian);
enc.encodeU32Packed(0, 300);
enc.encodeBool(3, true);

// Encoder with cursor (recommended)
BufferEncoderCursor enc(buf, sizeof(buf));
enc.encodeU32Packed(300);
enc.encodeBool(true);
enc.encodeString("hello");

// Decoder with cursor
BufferDecoderCursor dec(buf, sizeof(buf));
uint32_t value;
bool flag;
char str[32];
dec.decodeU32Packed(value);
dec.decodeBool(flag);
dec.decodeString(str, sizeof(str));
```

---

## API

### Construction

```cpp
BufferEncoder(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian);
BufferDecoder(const void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian);

// Cursor variants — same parameters plus internal pos = 0
BufferEncoderCursor(void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian);
BufferDecoderCursor(const void* buffer, size_t size, Endianness endianness = Endianness::LittleEndian);
```

---

### BufferEncoder / BufferDecoder

All methods take an explicit `pos` (byte offset into the buffer) and return the number of bytes consumed, or `0` on failure (out of bounds).

| Method | Description |
|--------|-------------|
| `encodeBool(pos, bool)` / `decodeBool(pos, bool&)` | 1 byte: `0x01` = true, `0x00` = false |
| `encodeIntegral<T>(pos, T)` / `decodeIntegral<T>(pos, T&)` | Any integer type, respects endianness |
| `encodeU32Packed(pos, uint32_t)` / `decodeU32Packed(pos, uint32_t&)` | Variable-length encoding, 1–4 bytes (see below) |
| `encodeBuffer(pos, void*, size)` / `decodeBuffer(pos, void*, size)` | Raw bytes copy |
| `encodeBufferWithLength(pos, void*, size)` / `decodeBufferWithLength(pos, void*, size&)` | Prepends `uint16_t` length, then raw bytes |
| `encodeString(pos, char*)` / `decodeString(pos, char*, maxLen)` | Null-terminated string |

---

### BufferEncoderCursor / BufferDecoderCursor

Same methods as above but **without the `pos` argument** — position is managed internally. Return `bool`: `true` on success, `false` on failure.

```cpp
bool encodeBool(bool data);
template <typename T> bool encodeIntegral(T value);
bool encodeU32Packed(uint32_t data);
bool encodeBuffer(void* data, size_t dataSize);
bool encodeBufferWithLength(void* data, size_t dataSize);
bool encodeString(char* data);

bool decodeBool(bool& data);
template <typename T> bool decodeIntegral(T& value);
bool decodeU32Packed(uint32_t& data);
bool decodeBuffer(void* data, size_t dataSize);
bool decodeBufferWithLength(void* data, size_t& dataSize);
bool decodeString(char* data, size_t maxLen);
```

Position control:

```cpp
size_t getPos() const;
void setPos(size_t pos);
void resetPos();
```

---

## Variable-length encoding (U32Packed)

`encodeU32Packed` / `decodeU32Packed` encode a `uint32_t` using 7 bits per byte. The MSB of each byte is a continuation flag.

| Value range | Bytes used |
|-------------|------------|
| 0 – 127 | 1 |
| 128 – 16 383 | 2 |
| 16 384 – 2 097 151 | 3 |
| 2 097 152 – 268 435 455 | 4 |

```
value = 300 (0b100101100)
→ byte 0: 0b10101100  (bits 0–6, continuation bit set)
→ byte 1: 0b00000010  (bits 7–13, no continuation)
```

---

## Error handling

All methods return `0` (low-level) or `false` (cursor) when the operation would exceed the buffer bounds. No exceptions are thrown. Typical usage pattern with the cursor:

```cpp
BufferDecoderCursor dec(buf, len);
uint32_t id;
uint16_t payloadLen;

if (!dec.decodeU32Packed(id))        return ParseError::UnexpectedEnd;
if (!dec.decodeIntegral(payloadLen)) return ParseError::UnexpectedEnd;
```

---

## Endianness

Pass `Endianness::LittleEndian` (default) or `Endianness::BigEndian` to the constructor. Applies to all `encodeIntegral` / `decodeIntegral` calls. `encodeU32Packed` is byte-order-independent by design.

---

## Thread safety

None. Each encoder/decoder instance owns its position state. Use separate instances per thread or synchronize externally.