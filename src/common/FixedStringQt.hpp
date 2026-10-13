#pragma once

#include "Header.hpp"

#include <QAnyStringView>
#include <QByteArray>
#include <QByteArrayView>
#include <QLatin1StringView>
#include <QString>
#include <QUtf8StringView>
#include <QtTypes>

#include <cstddef>
#include <limits>

namespace fixed_string::qt {

	namespace detail {

		template <std::size_t N>
		[[nodiscard]] constexpr qsizetype qtSize(const FixedString<N>& str) noexcept
		{
			static_assert(
				N - 1 <= static_cast<std::size_t>(std::numeric_limits<qsizetype>::max()),
				"FixedString is too large to be represented by Qt's qsizetype.");

			return static_cast<qsizetype>(str.size());
		}

	} // namespace detail

	// -----------------------------------------------------------------------------
	// Zero-copy Qt views
	// -----------------------------------------------------------------------------
	// The returned view refers directly to FixedString::data_.
	// Therefore the FixedString object must outlive the view.
	//
	// Rvalue overloads are deleted deliberately to prevent accidentally creating
	// a view that immediately dangles, e.g. asUtf8View(concat("A", "B")).

	template <std::size_t N>
	[[nodiscard]] constexpr QLatin1StringView
		asLatin1View(const FixedString<N>& str) noexcept
	{
		return QLatin1StringView{ str.constData(), detail::qtSize(str) };
	}

	template <std::size_t N>
	QLatin1StringView asLatin1View(const FixedString<N>&&) = delete;


	template <std::size_t N>
	[[nodiscard]] constexpr QUtf8StringView
		asUtf8View(const FixedString<N>& str) noexcept
	{
		return QUtf8StringView{ str.constData(), detail::qtSize(str) };
	}

	template <std::size_t N>
	QUtf8StringView asUtf8View(const FixedString<N>&&) = delete;


	template <std::size_t N>
	[[nodiscard]] constexpr QByteArrayView
		asByteArrayView(const FixedString<N>& str) noexcept
	{
		return QByteArrayView{ str.constData(), detail::qtSize(str) };
	}

	template <std::size_t N>
	QByteArrayView asByteArrayView(const FixedString<N>&&) = delete;


	// QAnyStringView interprets char data as UTF-8, which matches Qt's normal
	// semantics for 8-bit char strings. Use asLatin1View() when Latin-1 semantics
	// are required instead.
	template <std::size_t N>
	[[nodiscard]] constexpr QAnyStringView
		asAnyStringView(const FixedString<N>& str) noexcept
	{
		return QAnyStringView{ str.constData(), detail::qtSize(str) };
	}

	template <std::size_t N>
	QAnyStringView asAnyStringView(const FixedString<N>&&) = delete;


	// -----------------------------------------------------------------------------
	// Owning Qt strings
	// -----------------------------------------------------------------------------
	// These create independent Qt objects and therefore may allocate/copy.

	// Default QString conversion follows Qt's usual semantics for char data: UTF-8.
	template <std::size_t N>
	[[nodiscard]] QString toQString(const FixedString<N>& str)
	{
		return QString::fromUtf8(str.constData(), detail::qtSize(str));
	}

	// Explicit Latin-1 conversion for ASCII/Latin-1 data such as SQL text.
	template <std::size_t N>
	[[nodiscard]] QString toQStringLatin1(const FixedString<N>& str)
	{
		return QString::fromLatin1(str.constData(), detail::qtSize(str));
	}

	// Raw bytes, preserving the exact byte sequence stored by FixedString.
	template <std::size_t N>
	[[nodiscard]] QByteArray toQByteArray(const FixedString<N>& str)
	{
		return QByteArray{ str.constData(), detail::qtSize(str) };
	}

} // namespace fixed_string::qt
