#pragma once
#ifndef FIXED_STRING_MACROS_HPP
#define FIXED_STRING_MACROS_HPP
#include "../detail/FixedStringImpl.hpp"

// Opt-in, constant-expression bridges. Variadic arguments preserve template
// commas; the expression appears once, and is never evaluated at runtime.
#define FIXED_STATIC(...) \
    (::fixed_string::detail::materialize<::fixed_string::detail::makeKey((__VA_ARGS__))>())

#define FIXED_DATA(...) \
    (FIXED_STATIC(__VA_ARGS__).constData())
#endif