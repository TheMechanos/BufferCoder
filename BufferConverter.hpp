#pragma once

#include <BufferDecoder.hpp>
#include <BufferEncoder.hpp>

#include <type_traits>

// BufferConverter<T> - ArduinoJson-style serialization hooks.
//
// For user types write free functions (found via ADL):
//     bool encodeTo(BufferEncoderCursor& enc, const T& value);
//     bool decodeFrom(BufferDecoderCursor& dec, T& value);
// or specialize BufferConverter<T> with static encode()/decode() for types
// you cannot add free functions for.
//
// C++11 compatible, uses only <type_traits> from the standard library.

namespace bufferCoderDetail {

// C++11-safe void_t (std::void_t is C++17; struct form avoids CWG 1558 on older compilers)
template <typename...> struct MakeVoid {
    typedef void type;
};

// Pointer dereference in decltype is unevaluated - replaces std::declval without <utility>
template <typename T, typename = void> struct HasEncodeTo : std::false_type { };

template <typename T>
struct HasEncodeTo<T,
    typename MakeVoid<decltype(encodeTo(*static_cast<BufferEncoderCursor*>(nullptr),
                                        *static_cast<const T*>(nullptr)))>::type> : std::true_type { };

template <typename T, typename = void> struct HasDecodeFrom : std::false_type { };

template <typename T>
struct HasDecodeFrom<T,
    typename MakeVoid<decltype(decodeFrom(*static_cast<BufferDecoderCursor*>(nullptr),
                                          *static_cast<T*>(nullptr)))>::type> : std::true_type { };

// Tag dispatch instead of if constexpr - the true_type overload body is instantiated
// only when the hook exists, so a missing hook produces just the static_assert below.
template <typename T> bool callEncodeTo(BufferEncoderCursor& enc, const T& value, std::true_type) {
    return encodeTo(enc, value);
}
template <typename T> bool callEncodeTo(BufferEncoderCursor&, const T&, std::false_type) { return false; }

template <typename T> bool callDecodeFrom(BufferDecoderCursor& dec, T& value, std::true_type) {
    return decodeFrom(dec, value);
}
template <typename T> bool callDecodeFrom(BufferDecoderCursor&, T&, std::false_type) { return false; }

} // namespace bufferCoderDetail


// Primary template - forwards to user's encodeTo()/decodeFrom().
// Member functions are instantiated only when used, so a type may provide just one direction.
template <typename T, typename Enable> struct BufferConverter {
    static bool encode(BufferEncoderCursor& enc, const T& value) {
        typedef bufferCoderDetail::HasEncodeTo<T> Has;
        static_assert(Has::value,
            "BufferConverter: no bool encodeTo(BufferEncoderCursor&, const T&) found for this type "
            "- define it or specialize BufferConverter<T>");
        return bufferCoderDetail::callEncodeTo(enc, value, std::integral_constant<bool, Has::value>());
    }

    static bool decode(BufferDecoderCursor& dec, T& value) {
        typedef bufferCoderDetail::HasDecodeFrom<T> Has;
        static_assert(Has::value,
            "BufferConverter: no bool decodeFrom(BufferDecoderCursor&, T&) found for this type "
            "- define it or specialize BufferConverter<T>");
        return bufferCoderDetail::callDecodeFrom(dec, value, std::integral_constant<bool, Has::value>());
    }
};


// ---------- built-in converters ----------

template <> struct BufferConverter<bool> {
    static bool encode(BufferEncoderCursor& enc, const bool& value) { return enc.encodeBool(value); }
    static bool decode(BufferDecoderCursor& dec, bool& value) { return dec.decodeBool(value); }
};

template <typename T>
struct BufferConverter<T,
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type> {
    static bool encode(BufferEncoderCursor& enc, const T& value) { return enc.encodeIntegral(value); }
    static bool decode(BufferDecoderCursor& dec, T& value) { return dec.decodeIntegral(value); }
};

// Enums are stored as their underlying type (sizeof(underlying type) bytes)
template <typename T>
struct BufferConverter<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    typedef typename std::underlying_type<T>::type Underlying;

    static bool encode(BufferEncoderCursor& enc, const T& value) {
        return enc.encodeIntegral(static_cast<Underlying>(value));
    }

    static bool decode(BufferDecoderCursor& dec, T& value) {
        Underlying raw = Underlying();
        if (!dec.decodeIntegral(raw)) return false;
        value = static_cast<T>(raw);
        return true;
    }
};

// float / double - raw IEEE 754 bits, byte order follows the coder's endianness.
// Size follows sizeof(T): note that double is 4 bytes on AVR and 8 bytes on most other targets.
template <typename T>
struct BufferConverter<T,
    typename std::enable_if<std::is_same<T, float>::value || std::is_same<T, double>::value>::type> {
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "BufferConverter: unsupported floating point size");
    typedef typename std::conditional<sizeof(T) == 4, uint32_t, uint64_t>::type Bits;

    static bool encode(BufferEncoderCursor& enc, const T& value) {
        Bits bits;
        memcpy(&bits, &value, sizeof(bits));
        return enc.encodeIntegral(bits);
    }

    static bool decode(BufferDecoderCursor& dec, T& value) {
        Bits bits = 0;
        if (!dec.decodeIntegral(bits)) return false;
        memcpy(&value, &bits, sizeof(value));
        return true;
    }
};

// Fixed-size C arrays - N elements in order, no length prefix
template <typename T, size_t N> struct BufferConverter<T[N]> {
    static bool encode(BufferEncoderCursor& enc, const T (&value)[N]) {
        for (size_t i = 0; i < N; i++) {
            if (!enc.encode(value[i])) return false;
        }
        return true;
    }

    static bool decode(BufferDecoderCursor& dec, T (&value)[N]) {
        for (size_t i = 0; i < N; i++) {
            if (!dec.decode(value[i])) return false;
        }
        return true;
    }
};
