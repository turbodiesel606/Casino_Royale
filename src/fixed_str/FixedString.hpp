#pragma once
#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP
// Public core facade. Include this header for FixedString, FixedUtf8String,
// FixedUtf16String, FixedByteString, concat, operator+, FIXED_STATIC and FIXED_DATA.
// Requires C++20 and UTF-8 ordinary string literals; has no Qt dependency.
#include "detail/FixedStringImpl.hpp"
#include "macros/FixedStringMacros.hpp"

#endif