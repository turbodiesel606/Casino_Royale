#pragma once
#ifndef FIXED_STRING_QT_MACROS_HPP
#define FIXED_STRING_QT_MACROS_HPP
#include "FixedStringMacros.hpp"
#include "../detail/FixedStringQtDetail.hpp"

// The caller's expression occurs once, captured before the NTTP conversion.
// Qt only observes static storage; all validation and conversion is consteval.
#define FIXED_QSTRING(...) \
    (::fixed_string::qt::asQString(::fixed_string::detail::materialize< \
        ::fixed_string::qt::detail::makeQStringViewKey< \
            ::fixed_string::detail::makeKey((__VA_ARGS__))>()>()))
#define FIXED_QSTRING_VIEW(...) \
    (::fixed_string::qt::asQStringView(::fixed_string::detail::materialize< \
        ::fixed_string::qt::detail::makeQStringViewKey< \
            ::fixed_string::detail::makeKey((__VA_ARGS__))>()>()))
#define FIXED_UTF8_VIEW(...) \
    (::fixed_string::qt::asUtf8View(::fixed_string::detail::materialize< \
        ::fixed_string::qt::detail::makeUtf8ViewKey< \
            ::fixed_string::detail::makeKey((__VA_ARGS__))>()>()))
#define FIXED_ANY_QSTRING_VIEW(...) \
    (::fixed_string::qt::asAnyStringView(FIXED_STATIC(__VA_ARGS__)))
#define FIXED_LATIN1_VIEW(...) \
    (::fixed_string::qt::asLatin1View(::fixed_string::detail::materialize< \
        ::fixed_string::qt::detail::makeLatin1ViewKey< \
            ::fixed_string::detail::makeKey((__VA_ARGS__))>()>()))
#define FIXED_BYTE_ARRAY_VIEW(...) \
    (::fixed_string::qt::asByteArrayView(::fixed_string::detail::materialize< \
        ::fixed_string::qt::detail::makeByteArrayViewKey< \
            ::fixed_string::qt::detail::captureByteArrayInput((__VA_ARGS__))>()>()))
#endif 