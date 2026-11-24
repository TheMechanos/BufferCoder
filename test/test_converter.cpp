// Host test for BufferConverter.
// Build (check both the minimum and a modern standard):
//   g++ -std=c++11 -Wall -Wextra -I. test/test_converter.cpp -o test_converter && ./test_converter
//   g++ -std=c++17 -Wall -Wextra -I. test/test_converter.cpp -o test_converter && ./test_converter

#include <BufferCoder.hpp>

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            failures++;                                                   \
        }                                                                 \
    } while (0)


// ---------- user types (global namespace, ADL hooks) ----------

enum class Mode : uint8_t { A = 1, B = 7 };

struct Point {
    int16_t x;
    int16_t y;
};

struct Frame {
    uint16_t id;
    Mode mode;
    Point pos;
};

bool encodeTo(BufferEncoderCursor& enc, const Point& p) { return enc.encode(p.x) && enc.encode(p.y); }
bool decodeFrom(BufferDecoderCursor& dec, Point& p) { return dec.decode(p.x) && dec.decode(p.y); }

bool encodeTo(BufferEncoderCursor& enc, const Frame& f) {
    return enc.encode(f.id) && enc.encode(f.mode) && enc.encode(f.pos);
}
bool decodeFrom(BufferDecoderCursor& dec, Frame& f) {
    return dec.decode(f.id) && dec.decode(f.mode) && dec.decode(f.pos);
}

// Encode-only type - no decodeFrom on purpose
struct Ping {
    uint8_t seq;
};

bool encodeTo(BufferEncoderCursor& enc, const Ping& p) { return enc.encode(p.seq); }


// ---------- third-party type without ADL hooks - converter specialization ----------

namespace ext {
struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};
} // namespace ext

template <> struct BufferConverter<ext::Rgb> {
    static bool encode(BufferEncoderCursor& enc, const ext::Rgb& c) {
        return enc.encode(c.r) && enc.encode(c.g) && enc.encode(c.b);
    }
    static bool decode(BufferDecoderCursor& dec, ext::Rgb& c) {
        return dec.decode(c.r) && dec.decode(c.g) && dec.decode(c.b);
    }
};


// ---------- Task 1: core, bool, integral, enum ----------

static void integralLittleEndian() {
    uint8_t buf[16] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(uint16_t{0x1234}));
    CHECK(buf[0] == 0x34 && buf[1] == 0x12 && enc.getPos() == 2);
}

static void signedRoundtrip() {
    uint8_t buf[16] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(int32_t{-5}));

    BufferDecoderCursor dec(buf, sizeof(buf));
    int32_t v = 0;
    CHECK(dec.decode(v));
    CHECK(v == -5);
}

static void boolIsOneByte() {
    uint8_t buf[16] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(true));
    CHECK(enc.getPos() == 1 && buf[0] == 0x01);

    BufferDecoderCursor dec(buf, sizeof(buf));
    bool v = false;
    CHECK(dec.decode(v));
    CHECK(v == true);
}

static void enumUsesUnderlyingType() {
    uint8_t buf[16] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(Mode::B));
    CHECK(enc.getPos() == 1 && buf[0] == 7);

    BufferDecoderCursor dec(buf, sizeof(buf));
    Mode m = Mode::A;
    CHECK(dec.decode(m));
    CHECK(m == Mode::B);
}

static void nestedUserTypeRoundtrip() {
    uint8_t buf[16] = {};
    const Frame in{0xBEEF, Mode::A, {-3, 300}};

    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(in));
    CHECK(enc.getPos() == 7);

    BufferDecoderCursor dec(buf, sizeof(buf));
    Frame out{};
    CHECK(dec.decode(out));
    CHECK(dec.getPos() == 7);
    CHECK(out.id == 0xBEEF && out.mode == Mode::A && out.pos.x == -3 && out.pos.y == 300);
}

static void nestedFailurePropagates() {
    uint8_t buf[5] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(!enc.encode(Frame{1, Mode::B, {2, 3}}));
    CHECK(enc.hasAnyErrors());
}

static void encodeOnlyTypeCompiles() {
    uint8_t buf[4] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(Ping{9}));
    CHECK(buf[0] == 9);
}


// ---------- Task 2: float, double, arrays ----------

static void floatLittleEndianLayout() {
    uint8_t buf[8] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(1.0f));
    CHECK(enc.getPos() == 4);
    CHECK(buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x80 && buf[3] == 0x3F);

    BufferDecoderCursor dec(buf, sizeof(buf));
    float v = 0.0f;
    CHECK(dec.decode(v));
    CHECK(v == 1.0f);
}

static void floatBigEndianLayout() {
    uint8_t buf[8] = {};
    BufferEncoderCursor enc(buf, sizeof(buf), BufferCoderCommon::Endianness::BigEndian);
    CHECK(enc.encode(1.0f));
    CHECK(buf[0] == 0x3F && buf[1] == 0x80 && buf[2] == 0x00 && buf[3] == 0x00);
}

static void doubleRoundtrip() {
    uint8_t buf[16] = {};
    const double in = -123.456;
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(in));
    CHECK(enc.getPos() == 8);

    BufferDecoderCursor dec(buf, sizeof(buf));
    double out = 0.0;
    CHECK(dec.decode(out));
    CHECK(memcmp(&in, &out, sizeof(double)) == 0);
}

static void cArrayElementwise() {
    uint8_t buf[16] = {};
    const uint16_t a[3] = {1, 2, 0x0304};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(a));
    CHECK(enc.getPos() == 6);
    CHECK(buf[4] == 0x04 && buf[5] == 0x03);

    BufferDecoderCursor dec(buf, sizeof(buf));
    uint16_t b[3] = {};
    CHECK(dec.decode(b));
    CHECK(b[0] == a[0] && b[1] == a[1] && b[2] == a[2]);
}

static void arrayOfUserTypes() {
    uint8_t buf[16] = {};
    const Point in[2] = {{1, -1}, {1000, -1000}};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(in));
    CHECK(enc.getPos() == 8);

    BufferDecoderCursor dec(buf, sizeof(buf));
    Point out[2] = {};
    CHECK(dec.decode(out));
    CHECK(out[0].x == 1 && out[0].y == -1 && out[1].x == 1000 && out[1].y == -1000);
}

static void charArrayIsRawBytes() {
    uint8_t buf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const char name[4] = "abc";
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(name));
    CHECK(enc.getPos() == 4);
    CHECK(buf[0] == 'a' && buf[2] == 'c' && buf[3] == 0x00 && buf[4] == 0xFF);
}


// ---------- Task 3: specialization escape hatch ----------

static void specializedConverterRoundtrip() {
    uint8_t buf[8] = {};
    BufferEncoderCursor enc(buf, sizeof(buf));
    CHECK(enc.encode(ext::Rgb{1, 2, 3}));
    CHECK(enc.getPos() == 3);

    BufferDecoderCursor dec(buf, sizeof(buf));
    ext::Rgb out{};
    CHECK(dec.decode(out));
    CHECK(out.r == 1 && out.g == 2 && out.b == 3);
}


int main() {
    integralLittleEndian();
    signedRoundtrip();
    boolIsOneByte();
    enumUsesUnderlyingType();
    nestedUserTypeRoundtrip();
    nestedFailurePropagates();
    encodeOnlyTypeCompiles();

    floatLittleEndianLayout();
    floatBigEndianLayout();
    doubleRoundtrip();
    cArrayElementwise();
    arrayOfUserTypes();
    charArrayIsRawBytes();

    specializedConverterRoundtrip();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d CHECK(S) FAILED\n", failures);
    return 1;
}
