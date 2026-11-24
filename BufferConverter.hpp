#pragma once

#include <BufferDecoder.hpp>
#include <BufferEncoder.hpp>

#include <type_traits>
#include <utility>

// BufferConverter<T> - ArduinoJson-style serialization hooks.
//
// For user types write free functions (found via ADL):
//     bool encodeTo(BufferEncoderCursor& enc, const T& value);
//     bool decodeFrom(BufferDecoderCursor& dec, T& value);
// or specialize BufferConverter<T> with static encode()/decode() for types
// you cannot add free functions for.

namespace bufferCoderDetail {

template <typename> constexpr bool alwaysFalse = false;

template <typename T, typename = void> struct HasEncodeTo : std::false_type { };

template <typename T>
struct HasEncodeTo<T, std::void_t<decltype(encodeTo(std::declval<BufferEncoderCursor&>(), std::declval<const T&>()))>>
    : std::true_type { };

template <typename T, typename = void> struct HasDecodeFrom : std::false_type { };

template <typename T>
struct HasDecodeFrom<T, std::void_t<decltype(decodeFrom(std::declval<BufferDecoderCursor&>(), std::declval<T&>()))>>
    : std::true_type { };

} // namespace bufferCoderDetail


// Primary template - forwards to user's encodeTo()/decodeFrom().
// Member functions are instantiated only when used, so a type may provide just one direction.
template <typename T, typename Enable> struct BufferConverter {
    static bool encode(BufferEncoderCursor& enc, const T& value) {
        if constexpr (bufferCoderDetail::HasEncodeTo<T>::value) {
            return encodeTo(enc, value);
        } else {
            static_assert(bufferCoderDetail::alwaysFalse<T>,
                "BufferConverter: no bool encodeTo(BufferEncoderCursor&, const T&) found for this type "
                "- define it or specialize BufferConverter<T>");
            return false;
        }
    }

    static bool decode(BufferDecoderCursor& dec, T& value) {
        if constexpr (bufferCoderDetail::HasDecodeFrom<T>::value) {
            return decodeFrom(dec, value);
        } else {
            static_assert(bufferCoderDetail::alwaysFalse<T>,
                "BufferConverter: no bool decodeFrom(BufferDecoderCursor&, T&) found for this type "
                "- define it or specialize BufferConverter<T>");
            return false;
        }
    }
};


// ---------- built-in converters ----------

template <> struct BufferConverter<bool> {
    static bool encode(BufferEncoderCursor& enc, const bool& value) { return enc.encodeBool(value); }
    static bool decode(BufferDecoderCursor& dec, bool& value) { return dec.decodeBool(value); }
};

template <typename T>
struct BufferConverter<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static bool encode(BufferEncoderCursor& enc, const T& value) { return enc.encodeIntegral(value); }
    static bool decode(BufferDecoderCursor& dec, T& value) { return dec.decodeIntegral(value); }
};

// Enums are stored as their underlying type (sizeof(std::underlying_type_t<T>) bytes)
template <typename T> struct BufferConverter<T, std::enable_if_t<std::is_enum_v<T>>> {
    using Underlying = std::underlying_type_t<T>;

    static bool encode(BufferEncoderCursor& enc, const T& value) {
        return enc.encodeIntegral(static_cast<Underlying>(value));
    }

    static bool decode(BufferDecoderCursor& dec, T& value) {
        Underlying raw{};
        if (!dec.decodeIntegral(raw)) return false;
        value = static_cast<T>(raw);
        return true;
    }
};
