#pragma once
#ifndef FIXED_STRING_QT_HPP
#define FIXED_STRING_QT_HPP
// Public Qt facade, including the complete core API.
// Use FIXED_QSTRING for a zero-copy QString backed by static UTF-16 storage,
// FIXED_*_VIEW for persistent zero-copy views,
// and qt::toQString / qt::toQStringLatin1 / qt::toQByteArray
// when an independent owning Qt value is required.
#include "FixedString.hpp"
#include "detail/FixedStringQtAdapter.hpp"
#include "macros/FixedStringQtMacros.hpp"

#endif
