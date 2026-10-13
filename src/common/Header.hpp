#pragma once

#include <concepts>
#include <cstddef>
#include <ostream>
#include <type_traits>

template <std::size_t N>
class FixedString
{
	static_assert(
		N > 0, "FixedString size must include a trailing null terminator.");

public:
	consteval FixedString(const char(&str)[N])
	{
		while (size_ < N - 1 && str[size_] != '\0') {
			data_[size_] = str[size_];
			++size_;
		}

		data_[size_] = '\0';
	}

	[[nodiscard]] constexpr std::size_t size() const noexcept
	{
		return size_;
	}

	[[nodiscard]] constexpr const char* constData() const noexcept
	{
		return data_;
	}

private:
	char data_[N]{};
	std::size_t size_{};
};

template <typename T>
struct IsFixedString : std::false_type
{
};

template <std::size_t N>
struct IsFixedString<FixedString<N>> : std::true_type
{
};

template <typename T>
concept FixedStringType =
IsFixedString<std::remove_cvref_t<T>>::value;

template <typename T>
concept CharArray =
std::is_array_v<std::remove_reference_t<T>>
&& std::same_as<
	std::remove_cv_t<
	std::remove_extent_t<
	std::remove_reference_t<T>>>,
	char>
	&& !std::is_volatile_v<
	std::remove_extent_t<
	std::remove_reference_t<T>>>;

template <typename T>
concept FixedStringCompatible =
FixedStringType<T> || CharArray<T>;

template <std::size_t N>
consteval auto toFixedString(const char(&str)[N])
{
	return FixedString{ str };
}

template <std::size_t N>
consteval const FixedString<N>& toFixedString(const FixedString<N>& str)
{
	return str;
}

// FixedString + FixedString
//
// N1/N2 describe storage capacity, while size() is the logical string length.
// This matters for literals containing an embedded '\0', e.g. "ABC\0DEF":
// only "ABC" participates in concatenation.
template <std::size_t N1, std::size_t N2>
consteval auto operator+(
	const FixedString<N1>& lhs,
	const FixedString<N2>& rhs)
{
	char result[N1 + N2 - 1]{};
	std::size_t pos = 0;

	for (std::size_t i = 0; i < lhs.size(); ++i)
		result[pos++] = lhs.constData()[i];

	for (std::size_t i = 0; i < rhs.size(); ++i)
		result[pos++] = rhs.constData()[i];

	result[pos] = '\0';
	return FixedString{ result };
}

// FixedString + string literal / char array
template <std::size_t N1, std::size_t N2>
consteval auto operator+(
	const FixedString<N1>& lhs,
	const char(&rhs)[N2])
{
	return lhs + FixedString{ rhs };
}

// string literal / char array + FixedString
template <std::size_t N1, std::size_t N2>
consteval auto operator+(
	const char(&lhs)[N1],
	const FixedString<N2>& rhs)
{
	return FixedString{ lhs } + rhs;
}

// Mixed FixedString / string literals / char arrays.
template <FixedStringCompatible First, FixedStringCompatible... Rest>
consteval auto concat(const First& first, const Rest&... rest)
{
	return (
		toFixedString(first)
		+ ...
		+ toFixedString(rest)
		);
}
