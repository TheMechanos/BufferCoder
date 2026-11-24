#pragma once

// Minimal type traits - replacement for <type_traits>, so the library does not
// depend on the C++ standard library (e.g. Arduino AVR has none).
// Requires C++11. is_enum / underlying_type use compiler intrinsics
// (__is_enum, __underlying_type) supported by GCC, Clang and MSVC.

#include <stddef.h>
#include <stdint.h>

namespace bufferCoderDetail {

template <bool V> struct BoolConstant {
    static constexpr bool value = V;
};

typedef BoolConstant<true> TrueType;
typedef BoolConstant<false> FalseType;


template <bool Condition, typename T = void> struct EnableIf { };
template <typename T> struct EnableIf<true, T> {
    typedef T type;
};


// C++11-safe void_t (struct form avoids CWG 1558 on older compilers)
template <typename...> struct MakeVoid {
    typedef void type;
};


template <typename A, typename B> struct IsSame : FalseType { };
template <typename A> struct IsSame<A, A> : TrueType { };


template <typename T> struct RemoveCv {
    typedef T type;
};
template <typename T> struct RemoveCv<const T> {
    typedef T type;
};
template <typename T> struct RemoveCv<volatile T> {
    typedef T type;
};
template <typename T> struct RemoveCv<const volatile T> {
    typedef T type;
};


template <typename T> struct IsIntegralBase : FalseType { };
template <> struct IsIntegralBase<bool> : TrueType { };
template <> struct IsIntegralBase<char> : TrueType { };
template <> struct IsIntegralBase<signed char> : TrueType { };
template <> struct IsIntegralBase<unsigned char> : TrueType { };
template <> struct IsIntegralBase<wchar_t> : TrueType { };
template <> struct IsIntegralBase<char16_t> : TrueType { };
template <> struct IsIntegralBase<char32_t> : TrueType { };
#if defined(__cpp_char8_t)
template <> struct IsIntegralBase<char8_t> : TrueType { };
#endif
template <> struct IsIntegralBase<short> : TrueType { };
template <> struct IsIntegralBase<unsigned short> : TrueType { };
template <> struct IsIntegralBase<int> : TrueType { };
template <> struct IsIntegralBase<unsigned int> : TrueType { };
template <> struct IsIntegralBase<long> : TrueType { };
template <> struct IsIntegralBase<unsigned long> : TrueType { };
template <> struct IsIntegralBase<long long> : TrueType { };
template <> struct IsIntegralBase<unsigned long long> : TrueType { };

template <typename T> struct IsIntegral : IsIntegralBase<typename RemoveCv<T>::type> { };


template <typename T> struct IsEnum : BoolConstant<__is_enum(T)> { };

template <typename T> struct UnderlyingType {
    typedef __underlying_type(T) type;
};


// Unsigned integer type of a given byte size
template <size_t Size> struct UIntOfSize;
template <> struct UIntOfSize<1> {
    typedef uint8_t type;
};
template <> struct UIntOfSize<2> {
    typedef uint16_t type;
};
template <> struct UIntOfSize<4> {
    typedef uint32_t type;
};
template <> struct UIntOfSize<8> {
    typedef uint64_t type;
};


// Declaration only - for use in unevaluated contexts (decltype)
template <typename T> T&& declval() noexcept;

} // namespace bufferCoderDetail
