#pragma once
#ifndef FIXED_STRING_QT_ADAPTER_HPP
#define FIXED_STRING_QT_ADAPTER_HPP
#include "FixedStringImpl.hpp"

#include <QAnyStringView>
#include <QByteArray>
#include <QByteArrayView>
#include <QLatin1StringView>
#include <QString>
#include <QStringView>
#include <QUtf8StringView>
#include <QtTypes>

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace fixed_string::qt {

namespace detail {

template <typename T>
concept Text = FixedStringType<T> && std::remove_cvref_t<T>::encoding != Encoding::Bytes;
template <typename T>
concept Utf8Text = Text<T> && std::remove_cvref_t<T>::encoding == Encoding::Utf8;
template <typename T>
concept Utf16Text = Text<T> && std::remove_cvref_t<T>::encoding == Encoding::Utf16;
template <typename T>
concept Bytes = FixedStringType<T> && std::remove_cvref_t<T>::encoding == Encoding::Bytes;
template <typename T>
concept Narrow = FixedStringType<T> && std::same_as<fixed_string_char_t<T>, char>;
template <typename T>
concept OwningLvalue = OwningFixedStringType<T> && std::is_lvalue_reference_v<T&&>
    && !std::is_volatile_v<std::remove_reference_t<T>>;

template <FixedStringType T>
[[nodiscard]] constexpr qsizetype qtSize(const T& string) noexcept
{
    static_assert(std::remove_cvref_t<T>::capacity - 1
            <= static_cast<std::size_t>(std::numeric_limits<qsizetype>::max()),
        "Fixed string is too large for Qt's qsizetype.");
    return static_cast<qsizetype>(string.size());
}

} // namespace detail

// Persistent, zero-copy views. Stateless static-string temporaries are safe.

/*
    constexpr replaced with consteval for "as..." functions
*/

template <StaticStringType T> requires detail::Utf16Text<T>
[[nodiscard]] consteval QStringView asQStringView(const T& string) noexcept
{
    return QStringView{ string.constData(), detail::qtSize(string) };
}

template <StaticStringType T>
    requires detail::Utf16Text<T>
[[nodiscard]] QString asQString(const T& string)
{
    return QString::fromRawData(
        string.constData(),
        detail::qtSize(string)
    );
}

template <StaticStringType T> requires detail::Utf8Text<T>
[[nodiscard]] consteval QUtf8StringView asUtf8View(const T& string) noexcept
{
    return QUtf8StringView{ string.constData(), detail::qtSize(string) };
}

template <StaticStringType T> requires detail::Text<T>
[[nodiscard]] consteval QAnyStringView asAnyStringView(const T& string) noexcept
{
    if constexpr (detail::Utf16Text<T>)
        return QAnyStringView{ asQStringView(string) };
    else
        return QAnyStringView{ asUtf8View(string) };
}

template <StaticStringType T> requires detail::Bytes<T>
[[nodiscard]] consteval QLatin1StringView asLatin1View(const T& string) noexcept
{
    return QLatin1StringView{ string.constData(), detail::qtSize(string) };
}

template <StaticStringType T> requires detail::Narrow<T>
[[nodiscard]] consteval QByteArrayView asByteArrayView(const T& string) noexcept
{
    return QByteArrayView{ string.constData(), detail::qtSize(string) };
}

// Explicit borrowing accepts const/non-const owning lvalues only. The caller
// must keep that particular owner alive for as long as the view is used.

/*
    constexpr replaced with consteval for borrow functions
*/

template <detail::OwningLvalue T> requires detail::Utf16Text<T>
[[nodiscard]] consteval QStringView borrowQStringView(T&& string) noexcept
{
    return QStringView{ string.borrowedData(), detail::qtSize(string) };
}

template <detail::OwningLvalue T> requires detail::Utf8Text<T>
[[nodiscard]] consteval QUtf8StringView borrowUtf8View(T&& string) noexcept
{
    return QUtf8StringView{ string.borrowedData(), detail::qtSize(string) };
}

template <detail::OwningLvalue T> requires detail::Text<T>
[[nodiscard]] consteval QAnyStringView borrowAnyStringView(T&& string) noexcept
{
    if constexpr (detail::Utf16Text<T>)
        return QAnyStringView{ borrowQStringView(string) };
    else
        return QAnyStringView{ borrowUtf8View(string) };
}

template <detail::OwningLvalue T> requires detail::Bytes<T>
[[nodiscard]] consteval QLatin1StringView borrowLatin1View(T&& string) noexcept
{
    return QLatin1StringView{ string.borrowedData(), detail::qtSize(string) };
}

template <detail::OwningLvalue T> requires detail::Narrow<T>
[[nodiscard]] consteval QByteArrayView borrowByteArrayView(T&& string) noexcept
{
    return QByteArrayView{ string.borrowedData(), detail::qtSize(string) };
}

// Owning conversions copy/transcode at runtime and accept either representation,
// including temporaries. Their results have independent Qt-managed lifetimes.
template <detail::Text T>
[[nodiscard]] QString toQString(const T& string)
{
    if constexpr (detail::Utf16Text<T>)
        return QString::fromUtf16(fixed_string::detail::stringData(string), detail::qtSize(string));
    else
        return QString::fromUtf8(fixed_string::detail::stringData(string), detail::qtSize(string));
}

template <detail::Bytes T>
[[nodiscard]] QString toQStringLatin1(const T& string)
{
    return QString::fromLatin1(fixed_string::detail::stringData(string), detail::qtSize(string));
}

template <detail::Narrow T>
[[nodiscard]] QByteArray toQByteArray(const T& string)
{
    return QByteArray{ fixed_string::detail::stringData(string), detail::qtSize(string) };
}

} // namespace fixed_string::qt

#endif