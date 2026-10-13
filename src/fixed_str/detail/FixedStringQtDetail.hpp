#pragma once
#ifndef FIXED_STRING_QT_DETAIL_HPP
#define FIXED_STRING_QT_DETAIL_HPP
#include "FixedStringQtAdapter.hpp"

// Internal representation selection for the macro facade, not public API.
namespace fixed_string::qt::detail {

namespace core_detail = ::fixed_string::detail;

template <core_detail::StringKey Key>
    requires (Key.encoding != Encoding::Bytes)
[[nodiscard]] consteval auto makeQStringViewKey()
{
    if constexpr (Key.encoding == Encoding::Utf16)
        return Key;
    else
        return core_detail::transcodeKey<Key, core_detail::TranscodeTarget::Utf16>();
}

// Qt's byte and UTF-8 views use char storage. This is a code-unit copy, not
// Unicode transcoding; preserve every bit, including in char8_t input.
template <core_detail::StringKey Key, Encoding Kind>
    requires (Key.encoding != Encoding::Utf16
        && (Kind == Key.encoding || Kind == Encoding::Bytes))
[[nodiscard]] consteval auto makeNarrowViewKey()
{
    constexpr auto key = core_detail::normalizeKey(Key);
    constexpr auto size = core_detail::logicalSize(key.data);
    core_detail::StringKey<char, size + 1, Kind> result{};
    for (std::size_t i = 0; i < size; ++i)
        result.data[i] = core_detail::byteToChar(core_detail::utf8Byte(key.data, i));
    return result;
}

template <core_detail::StringKey Key>
    requires (Key.encoding != Encoding::Bytes)
[[nodiscard]] consteval auto makeUtf8ViewKey()
{
    if constexpr (Key.encoding == Encoding::Utf8)
        return makeNarrowViewKey<Key, Encoding::Utf8>();
    else
        return core_detail::transcodeKey<Key, core_detail::TranscodeTarget::Utf8>();
}

template <core_detail::StringKey Key>
[[nodiscard]] consteval auto makeLatin1ViewKey()
{
    if constexpr (Key.encoding == Encoding::Bytes)
        return core_detail::normalizeKey(Key);
    else
        return core_detail::transcodeKey<Key, core_detail::TranscodeTarget::Latin1>();
}

// Preserve the byte-array facade's existing raw-char-array contract. Text
// fixed strings retain their encoding during capture; char8_t arrays validate
// as UTF-8. UTF-16 has no implicit byte representation here.
template <std::size_t N>
[[nodiscard]] consteval auto captureByteArrayInput(const char (&value)[N])
{
    return core_detail::makeKey(FixedByteString<N>{ value });
}

template <std::size_t N>
[[nodiscard]] consteval auto captureByteArrayInput(const char8_t (&value)[N])
{
    return core_detail::makeKey(value);
}

template <FixedStringType T> requires (Narrow<T> || Utf8Text<T>)
[[nodiscard]] consteval auto captureByteArrayInput(const T& value)
{
    return core_detail::makeKey(value);
}

template <core_detail::StringKey Key> requires (Key.encoding != Encoding::Utf16)
[[nodiscard]] consteval auto makeByteArrayViewKey()
{
    return makeNarrowViewKey<Key, Encoding::Bytes>();
}

} // namespace fixed_string::qt::detail

#endif