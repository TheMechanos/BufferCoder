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

BufferConverter<T>   — encode()/decode() of custom types (see "Custom types")
```

`BufferCoderCommon` holds the buffer pointer, size, and endianness. Encoder and decoder inherit from it and add their respective read/write methods. The `Cursor` variants track the current position automatically.

---

## Quick start

```cpp
#include <BufferCoder.hpp>

uint8_t buf[64];

// Encoder with manual position
BufferEncoder rawEnc(buf, sizeof(buf), BufferCoderCommon::Endianness::LittleEndian);
size_t pos = 0;
pos += rawEnc.encodeU32Packed(pos, 300);
pos += rawEnc.encodeBool(pos, true);

// Encoder with cursor (recommended)
BufferEncoderCursor enc(buf, sizeof(buf));
enc.encodeU32Packed(300);
enc.encodeBool(true);
enc.skipBytes(2);           // reserved bytes, filled with 0x00
enc.encodeString("hello");

// Decoder with cursor
BufferDecoderCursor dec(buf, sizeof(buf));
uint32_t value;
bool flag;
char str[32];
dec.decodeU32Packed(value);
dec.decodeBool(flag);
dec.skipBytes(2);
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
| `encodeBuffer(pos, const void*, size)` / `decodeBuffer(pos, void*, size)` | Raw bytes copy. For `size == 0` returns `0` (nothing to copy) |
| `encodeBufferWithLength(pos, const void*, size)` / `decodeBufferWithLength(pos, void*, maxSize, size&)` | Prepends `uint16_t` length, then raw bytes. Encoding fails if `size > 65535`. Decoding fails if the stored length exceeds `maxSize` (destination capacity) or the source buffer. Empty payload is valid (2 bytes) |
| `encodeString(pos, const char*)` / `decodeString(pos, char*, maxLen)` | Null-terminated string |

---

### BufferEncoderCursor / BufferDecoderCursor

Same methods as above but **without the `pos` argument** — position is managed internally. Return `bool`: `true` on success, `false` on failure.

```cpp
bool encodeBool(bool data);
template <typename T> bool encodeIntegral(T value);
bool encodeU32Packed(uint32_t data);
bool encodeBuffer(const void* data, size_t dataSize);       // dataSize == 0 is a successful no-op
bool encodeBufferWithLength(const void* data, size_t dataSize);
bool encodeString(const char* data);

bool decodeBool(bool& data);
template <typename T> bool decodeIntegral(T& value);
bool decodeU32Packed(uint32_t& data);
bool decodeBuffer(void* data, size_t dataSize);             // dataSize == 0 is a successful no-op
bool decodeBufferWithLength(void* data, size_t maxSize, size_t& dataSize);
bool decodeString(char* data, size_t maxLen);
```

Skipping bytes:

```cpp
bool skipBytes(size_t bytes);  // encoder: writes 0x00 bytes, decoder: ignores bytes
```

Position control:

```cpp
size_t getPos() const;
void setPos(size_t pos);
void resetPos();
```

Error state:

```cpp
bool hasAnyErrors() const;  // true if any operation failed since construction or last clearErrors()
void clearErrors();          // reset the error flag
```

`hasAnyErrors()` is a sticky flag — once set it stays `true` regardless of subsequent successful operations. This lets you fire off a sequence of calls and check the result once at the end instead of after every step:

```cpp
BufferEncoderCursor enc(buf, sizeof(buf));
enc.encodeU32Packed(id);
enc.encodeIntegral(static_cast<uint16_t>(payloadLen));
enc.encodeBuffer(payload, payloadLen);

if (enc.hasAnyErrors()) return false;
send(buf, enc.getPos());
```

---

## Custom types

Inspired by ArduinoJson's custom converters: any type can be encoded/decoded with a single call by writing a pair of free functions for it. Nested types compose automatically. Requires `#include <BufferCoder.hpp>`.

The whole library needs only C++11 and does not use the C++ standard library (no `std::`) — it works on targets without an STL, e.g. Arduino AVR. Enum support relies on the `__is_enum` / `__underlying_type` compiler intrinsics (GCC, Clang, MSVC).

```cpp
template <typename T> bool BufferEncoderCursor::encode(const T& value);
template <typename T> bool BufferDecoderCursor::decode(T& value);
```

### Free functions (recommended)

Declare them in the same namespace as the type (found via argument-dependent lookup):

```cpp
#include <BufferCoder.hpp>

enum class Mode : uint8_t { Idle, Run };

struct Point { int16_t x, y; };
struct Frame { uint16_t id; Mode mode; Point pos; float temp; };

bool encodeTo(BufferEncoderCursor& enc, const Point& p) { return enc.encode(p.x) && enc.encode(p.y); }
bool decodeFrom(BufferDecoderCursor& dec, Point& p)     { return dec.decode(p.x) && dec.decode(p.y); }

bool encodeTo(BufferEncoderCursor& enc, const Frame& f) {
    return enc.encode(f.id) && enc.encode(f.mode) && enc.encode(f.pos) && enc.encode(f.temp);
}
bool decodeFrom(BufferDecoderCursor& dec, Frame& f) {
    return dec.decode(f.id) && dec.decode(f.mode) && dec.decode(f.pos) && dec.decode(f.temp);
}

// usage
BufferEncoderCursor enc(buf, sizeof(buf));
enc.encode(frame);

BufferDecoderCursor dec(buf, len);
Frame frame;
if (!dec.decode(frame)) return false;
```

Declare `encodeTo` / `decodeFrom` before the first `encode` / `decode` call for that type (typically right next to the type in its header) — the detection result is fixed at the first use.

A type may define only one direction (e.g. only `encodeTo` for a message that is never received). Calling `decode` on it then fails at compile time with a readable `static_assert`.

Instead of chaining with `&&` you can also call `encode`/`decode` unconditionally and check `hasAnyErrors()` at the end.

### Converter specialization

For types you cannot add free functions for (standard library, third-party code), specialize `BufferConverter<T>` in the global namespace:

```cpp
template <> struct BufferConverter<ext::Rgb> {
    static bool encode(BufferEncoderCursor& enc, const ext::Rgb& c) {
        return enc.encode(c.r) && enc.encode(c.g) && enc.encode(c.b);
    }
    static bool decode(BufferDecoderCursor& dec, ext::Rgb& c) {
        return dec.decode(c.r) && dec.decode(c.g) && dec.decode(c.b);
    }
};
```

### Built-in types

| Type | Encoding |
|------|----------|
| `bool` | 1 byte (`encodeBool`) |
| integral types | `sizeof(T)` bytes, coder endianness (`encodeIntegral`) |
| enums | underlying type — use a fixed one (`enum class X : uint8_t`) for a portable size |
| `float` / `double` | IEEE 754 bits as `uint32_t` / `uint64_t`, coder endianness |
| `T[N]` | N elements in order, no length prefix |

`double` is encoded with `sizeof(double)` bytes — on AVR that is 4 bytes, on ESP32/ARM/x86 8 bytes. Use `float` in messages exchanged between such platforms.

`char[N]` is encoded as N raw bytes, not as a string — use `encodeString` / `decodeString` for null-terminated strings.

---

## Variable-length encoding (U32Packed)

`encodeU32Packed` / `decodeU32Packed` encode a `uint32_t` using 7 bits per byte. The MSB of each byte is a continuation flag.

| Value range | Bytes used |
|-------------|------------|
| 0 – 127 | 1 |
| 128 – 16 383 | 2 |
| 16 384 – 2 097 151 | 3 |
| 2 097 152 – 268 435 455 | 4 |

Maximum encodable value is `BufferCoderCommon::U32PackedMax` (268 435 455). Larger values make `encodeU32Packed` fail. `decodeU32Packed` fails if the 4th byte still has the continuation bit set (malformed input).

```
value = 300 (0b100101100)
→ byte 0: 0b10101100  (bits 0–6, continuation bit set)
→ byte 1: 0b00000010  (bits 7–13, no continuation)
```

---

## Error handling

All methods return `0` (low-level) or `false` (cursor) when the operation would exceed the buffer bounds. No exceptions are thrown.

Two patterns are available with the cursor — check per call, or check once at the end:

```cpp
// Per-call — stop early on first error
BufferDecoderCursor dec(buf, len);
uint32_t id;
uint16_t payloadLen;

if (!dec.decodeU32Packed(id))        return ParseError::UnexpectedEnd;
if (!dec.decodeIntegral(payloadLen)) return ParseError::UnexpectedEnd;

// Deferred — fire all calls, check once at the end
BufferDecoderCursor dec(buf, len);
uint32_t id;
uint16_t payloadLen;
char name[32];

dec.decodeU32Packed(id);
dec.decodeIntegral(payloadLen);
dec.decodeString(name, sizeof(name));

if (dec.hasAnyErrors()) return ParseError::UnexpectedEnd;
```

---

## Endianness

Pass `BufferCoderCommon::Endianness::LittleEndian` (default) or `BufferCoderCommon::Endianness::BigEndian` to the constructor. Applies to all `encodeIntegral` / `decodeIntegral` calls. `encodeU32Packed` is byte-order-independent by design.

---

## Thread safety

None. Each encoder/decoder instance owns its position state. Use separate instances per thread or synchronize externally.
