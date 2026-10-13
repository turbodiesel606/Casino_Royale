#pragma once
#ifndef FIXED_STRING_IMPL_HPP
#define FIXED_STRING_IMPL_HPP

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

namespace fixed_string {

    template <typename T>
    concept FixedStringChar =
        std::same_as<T, char>
        || std::same_as<T, char8_t>
        || std::same_as<T, char16_t>;

    enum class Encoding { Utf8, Utf16, Bytes };

    namespace detail {

        // Storage-category tags are intentionally simple nested-type markers.
        // They avoid partial-specialization matching on StaticString<StringKey>,
        // which some IDE language services (notably Visual Studio IntelliSense)
        // can mis-evaluate even though the real compiler accepts the code.
        struct OwningStringTag final {};
        struct StaticStringTag final {};

        template <typename T>
        concept HasFixedStringStorageTag = requires {
            typename std::remove_cvref_t<T>::fixed_string_storage_tag;
        };

        // Ordinary text literals must already contain UTF-8 before validation runs.
        // IntelliSense uses a separate EDG front-end whose execution-charset model may
        // differ from the actual compiler, so keep this environment check for real builds.
#ifndef __INTELLISENSE__
        static_assert(
            sizeof("\u00E9") == 3
            && static_cast<unsigned char>("\u00E9"[0]) == 0xC3U
            && static_cast<unsigned char>("\u00E9"[1]) == 0xA9U,
            "fixed_string requires UTF-8 ordinary literals. "
            "Use /utf-8 (MSVC/clang-cl) or "
            "-finput-charset=UTF-8 -fexec-charset=UTF-8 (GCC/Clang), "
            "and save sources as UTF-8.");
#endif
        template <FixedStringChar CharT>
        inline constexpr Encoding textEncoding =
            std::same_as<CharT, char16_t> ? Encoding::Utf16 : Encoding::Utf8;

        template <FixedStringChar CharT, Encoding Kind>
        inline constexpr bool validEncoding =
            Kind == textEncoding<CharT>
            || (std::same_as<CharT, char> && Kind == Encoding::Bytes);

        struct DecodedCodePoint
        {
            std::uint32_t value{};
            std::size_t width{};
            bool valid{};
        };

        template <typename CharT>
            requires (std::same_as<CharT, char> || std::same_as<CharT, char8_t>)
        [[nodiscard]] constexpr unsigned int utf8Byte(
            const CharT* source, const std::size_t position) noexcept
        {
            return static_cast<unsigned char>(source[position]);
        }

        template <typename CharT>
            requires (std::same_as<CharT, char> || std::same_as<CharT, char8_t>)
        [[nodiscard]] constexpr DecodedCodePoint decodeUtf8(
            const CharT* source,
            const std::size_t size,
            const std::size_t position) noexcept
        {
            if (position >= size)
                return {};

            const auto lead = utf8Byte(source, position);
            if (lead <= 0x7FU)
                return { lead, 1, true };

            std::size_t width{};
            std::uint32_t value{};
            std::uint32_t minimum{};

            if (lead >= 0xC2U && lead <= 0xDFU) {
                width = 2;
                value = lead & 0x1FU;
                minimum = 0x80U;
            }
            else if (lead >= 0xE0U && lead <= 0xEFU) {
                width = 3;
                value = lead & 0x0FU;
                minimum = 0x800U;
            }
            else if (lead >= 0xF0U && lead <= 0xF4U) {
                width = 4;
                value = lead & 0x07U;
                minimum = 0x10000U;
            }
            else {
                return {};
            }

            if (width > size - position)
                return {};

            for (std::size_t offset = 1; offset < width; ++offset) {
                const auto continuation = utf8Byte(source, position + offset);
                if ((continuation & 0xC0U) != 0x80U)
                    return {};
                value = (value << 6U) | (continuation & 0x3FU);
            }

            if (value < minimum || value > 0x10FFFFU
                || (value >= 0xD800U && value <= 0xDFFFU)) {
                return {};
            }
            return { value, width, true };
        }

        [[nodiscard]] constexpr DecodedCodePoint decodeUtf16(
            const char16_t* source, const std::size_t size,
            const std::size_t position) noexcept
        {
            if (position >= size)
                return {};
            const std::uint32_t first = source[position];
            if (first >= 0xD800U && first <= 0xDBFFU) {
                if (size - position < 2)
                    return {};
                const std::uint32_t second = source[position + 1];
                if (second < 0xDC00U || second > 0xDFFFU)
                    return {};
                return { 0x10000U + ((first - 0xD800U) << 10U)
                    + (second - 0xDC00U), 2, true };
            }
            if (first >= 0xDC00U && first <= 0xDFFFU)
                return {};
            return { first, 1, true };
        }

        template <FixedStringChar CharT>
        [[nodiscard]] constexpr auto decodeText(
            const CharT* source, const std::size_t size,
            const std::size_t position) noexcept
        {
            if constexpr (std::same_as<CharT, char16_t>)
                return decodeUtf16(source, size, position);
            else
                return decodeUtf8(source, size, position);
        }

        // Preserve the byte's representation even when plain char is signed.
        [[nodiscard]] constexpr char byteToChar(const std::uint32_t byte) noexcept
        {
            return std::bit_cast<char>(static_cast<unsigned char>(byte));
        }

        // Conversion targets are separate from storage classification: Bytes
        // itself makes no promise that its contents represent Latin-1 text.
        enum class TranscodeTarget
        {
            Utf8,      
            Utf8Char8,  // char8_t UTF-8
            Utf16,
            Latin1
        };
        template <TranscodeTarget Target, typename Emit>
        constexpr void encodeCodePoint(const std::uint32_t value, Emit emit)
        {
            if constexpr (Target == TranscodeTarget::Latin1) {
                if (value > 0xFFU)
                    throw "fixed_string: text is not representable in Latin-1";
                emit(value);
            }
            else if constexpr (Target == TranscodeTarget::Utf16) {
                if (value <= 0xFFFFU) {
                    emit(value);
                }
                else {
                    const auto supplementary = value - 0x10000U;
                    emit(0xD800U + (supplementary >> 10U));
                    emit(0xDC00U + (supplementary & 0x3FFU));
                }
            }
            else {
                if (value <= 0x7FU) {
                    emit(value);
                }
                else if (value <= 0x7FFU) {
                    emit(0xC0U | (value >> 6U));
                    emit(0x80U | (value & 0x3FU));
                }
                else if (value <= 0xFFFFU) {
                    emit(0xE0U | (value >> 12U));
                    emit(0x80U | ((value >> 6U) & 0x3FU));
                    emit(0x80U | (value & 0x3FU));
                }
                else {
                    emit(0xF0U | (value >> 18U));
                    emit(0x80U | ((value >> 12U) & 0x3FU));
                    emit(0x80U | ((value >> 6U) & 0x3FU));
                    emit(0x80U | (value & 0x3FU));
                }
            }
        }

        // Both sizing and writing use this decoder/encoder pair. char and
        // char8_t feed the same UTF-8 decoder directly, without a temporary array.
        template <TranscodeTarget Target, FixedStringChar CharT, typename Emit>
        consteval void transcode(const CharT* source, const std::size_t size, Emit emit)
        {
            std::size_t position = 0;
            while (position < size) {
                const auto decoded = decodeText(source, size, position);
                if (!decoded.valid)
                    throw "fixed_string: invalid Unicode input";
                encodeCodePoint<Target>(decoded.value, emit);
                position += decoded.width;
            }
        }

        // Scan the full array: its final element is not assumed to be a terminator.
        template <FixedStringChar CharT, std::size_t N>
        [[nodiscard]] consteval std::size_t logicalSize(const CharT(&source)[N])
        {
            for (std::size_t i = 0; i < N; ++i) {
                if (source[i] == CharT{})
                    return i;
            }
            throw "fixed_string: unterminated input array";
        }

        template <FixedStringChar CharT, Encoding Kind>
        consteval void validate(const CharT* source, const std::size_t size)
        {
            static_assert(validEncoding<CharT, Kind>, "Invalid character/encoding pair.");
            if constexpr (Kind != Encoding::Bytes) {
                std::size_t position = 0;
                while (position < size) {
                    const auto decoded = decodeText(source, size, position);
                    if (!decoded.valid) {
                        if constexpr (Kind == Encoding::Utf8)
                            throw "fixed_string: invalid UTF-8 input";
                        else
                            throw "fixed_string: invalid UTF-16 input";
                    }
                    position += decoded.width;
                }
            }
        }

        // This structural key is internal. It copies code units, never source pointers.
        // Literal capture is deliberately unvalidated so _fbytes can opt out of Unicode
        // validation. Every path into StaticString validates and normalizes the key.
        template <FixedStringChar CharT, std::size_t Capacity,
            Encoding Kind = textEncoding<CharT>>
            struct StringKey
        {
            static_assert(Capacity > 0);
            static_assert(validEncoding<CharT, Kind>);
            using value_type = CharT;
            static constexpr std::size_t capacity = Capacity;
            static constexpr Encoding encoding = Kind;

            CharT data[Capacity]{};

            consteval StringKey() = default;

            consteval StringKey(const CharT(&source)[Capacity])
            {
                for (std::size_t i = 0; i < Capacity; ++i)
                    data[i] = source[i];
            }
        };

        template <FixedStringChar CharT, std::size_t N>
        StringKey(const CharT(&)[N]) -> StringKey<CharT, N>;

        template <FixedStringChar CharT, std::size_t N, Encoding Kind>
        [[nodiscard]] consteval auto normalizeKey(const StringKey<CharT, N, Kind>& key)
        {
            const auto size = logicalSize(key.data);
            validate<CharT, Kind>(key.data, size);
            StringKey<CharT, N, Kind> result{};
            for (std::size_t i = 0; i < size; ++i)
                result.data[i] = key.data[i];
            return result;
        }

        template <StringKey Key>
        consteval bool isNormalizedKey()
        {
            constexpr auto normalized = normalizeKey(Key);
            for (std::size_t i = 0; i < Key.capacity; ++i) {
                if (Key.data[i] != normalized.data[i])
                    return false;
            }
            return true;
        }

        template <StringKey Key, TranscodeTarget Target>
            requires (Key.encoding != Encoding::Bytes)
        [[nodiscard]] consteval std::size_t transcodedCapacity()
        {
            std::size_t capacity = 1; // Include the final null code unit.
            transcode<Target>(Key.data, logicalSize(Key.data),
                [&](std::uint32_t) constexpr {
                    if (capacity == std::numeric_limits<std::size_t>::max())
                        throw "fixed_string: transcoding capacity overflow";
                    ++capacity;
                });
            return capacity;
        }

        // The captured value is an NTTP so its exact encoded length can be
        // part of the result type. No pointer to a source owner is retained.
        template <StringKey Key, TranscodeTarget Target>
            requires (Key.encoding != Encoding::Bytes)
        [[nodiscard]] consteval auto transcodeKey()
        {
            using CharT = std::conditional_t<
                Target == TranscodeTarget::Utf16,
                char16_t,
                std::conditional_t<
                Target == TranscodeTarget::Utf8Char8,
                char8_t,
                char>>;            constexpr auto kind = Target == TranscodeTarget::Latin1
                ? Encoding::Bytes : textEncoding<CharT>;
            StringKey<CharT, transcodedCapacity<Key, Target>(), kind> result{};
            std::size_t position = 0;
            transcode<Target>(Key.data, logicalSize(Key.data),
                [&](const std::uint32_t unit) constexpr {
                    if constexpr (std::same_as<CharT, char>)
                        result.data[position++] = byteToChar(unit);
                    else
                        result.data[position++] = static_cast<CharT>(unit);
                });
            return result;
        }

    } // namespace detail

    template <FixedStringChar CharT, std::size_t Capacity,
        Encoding Kind = detail::textEncoding<CharT>>
        class BasicFixedString
    {
        static_assert(Capacity > 0,
            "BasicFixedString capacity must include a trailing null terminator.");
        static_assert(detail::validEncoding<CharT, Kind>);

    public:
        using fixed_string_storage_tag = detail::OwningStringTag;
        using value_type = CharT;
        using size_type = std::size_t;
        static constexpr size_type capacity = Capacity;
        static constexpr Encoding encoding = Kind;

        template <std::size_t SourceN>
            requires (SourceN <= Capacity)
        consteval BasicFixedString(const CharT(&source)[SourceN])
            : size_{ detail::logicalSize(source) }
        {
            detail::validate<CharT, Kind>(source, size_);
            for (size_type i = 0; i < size_; ++i)
                data_[i] = source[i];
        }

        template <std::size_t SourceN>
            requires (!std::same_as<CharT, char>&& SourceN <= Capacity)
        consteval BasicFixedString(const char(&source)[SourceN])
        {
            const auto inputSize = detail::logicalSize(source);
            detail::validate<char, Encoding::Utf8>(source, inputSize);
            if constexpr (std::same_as<CharT, char8_t>) {
                for (size_type i = 0; i < inputSize; ++i)
                    data_[i] = static_cast<char8_t>(static_cast<unsigned char>(source[i]));
                size_ = inputSize;
            }
            else {
                detail::transcode<detail::TranscodeTarget::Utf16>(source, inputSize,
                    [&](const std::uint32_t unit) constexpr {
                        data_[size_++] = static_cast<char16_t>(unit);
                    });
            }
        }

        [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

        // Explicit borrowing only. Even an lvalue's pointer must not outlive it.
        [[nodiscard]] constexpr const CharT* borrowedData() const& noexcept { return data_; }
        const CharT* borrowedData() && = delete;
        const CharT* borrowedData() const&& = delete;

    private:
        CharT data_[Capacity]{};
        size_type size_{};
    };

    template <FixedStringChar CharT, std::size_t N>
    BasicFixedString(const CharT(&)[N]) -> BasicFixedString<CharT, N>;

    // Alias-template deduction supplies CharT for narrow-to-Unicode construction.
    // Defaults make the otherwise non-deducible guide parameters valid on Clang;
    // the constraints exclude those defaults from direct BasicFixedString deduction.
    template <FixedStringChar CharT = char, std::size_t N>
        requires (!std::same_as<CharT, char>)
    BasicFixedString(const char(&)[N])->BasicFixedString<CharT, N>;

    // Only alias deduction supplies Kind here; direct BasicFixedString stays text.
    template <std::size_t N, Encoding Kind = Encoding::Utf8>
        requires (Kind == Encoding::Bytes)
    BasicFixedString(const char(&)[N])->BasicFixedString<char, N, Kind>;

    template <std::size_t N>
    using FixedString = BasicFixedString<char, N>;
    template <std::size_t N>
    using FixedUtf8String = BasicFixedString<char8_t, N>;
    template <std::size_t N>
    using FixedUtf16String = BasicFixedString<char16_t, N>;
    template <std::size_t N>
    using FixedByteString = BasicFixedString<char, N, Encoding::Bytes>;

    template <detail::StringKey Key>
    class StaticString
    {
        static_assert(detail::isNormalizedKey<Key>(), "StaticString requires a normalized key.");

    public:
        using fixed_string_storage_tag = detail::StaticStringTag;
        using value_type = typename decltype(Key)::value_type;
        using size_type = std::size_t;
        static constexpr size_type capacity = Key.capacity;
        static constexpr Encoding encoding = Key.encoding;

        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return detail::logicalSize(Key.data);
        }

        // Key names a C++20 template parameter object with static storage duration.
        [[nodiscard]] constexpr const value_type* constData() const noexcept { return Key.data; }
    };

    // Storage classification is tag-based instead of relying on partial
    // specializations that must decompose StaticString<Key>. This keeps the public
    // concepts equivalent while being substantially easier for IDE parsers.
    template <typename T>
    concept OwningFixedStringType =
        detail::HasFixedStringStorageTag<T>
        && std::same_as<
        typename std::remove_cvref_t<T>::fixed_string_storage_tag,
        detail::OwningStringTag>;

    template <typename T>
    concept StaticStringType =
        detail::HasFixedStringStorageTag<T>
        && std::same_as<
        typename std::remove_cvref_t<T>::fixed_string_storage_tag,
        detail::StaticStringTag>;

    template <typename T>
    concept FixedStringType = OwningFixedStringType<T> || StaticStringType<T>;

    // Keep the trait names for source compatibility, but implement them through
    // the tag-based concepts rather than class-template partial specializations.
    template <typename T>
    struct IsOwningFixedString : std::bool_constant<OwningFixedStringType<T>> {};

    template <typename T>
    struct IsStaticString : std::bool_constant<StaticStringType<T>> {};

    template <typename T>
    struct IsFixedString : std::bool_constant<FixedStringType<T>> {};
    template <FixedStringType T>
    using fixed_string_char_t = typename std::remove_cvref_t<T>::value_type;

    namespace detail {

        template <typename T>
        struct IsCharArray : std::false_type {};
        template <typename CharT, std::size_t N>
        struct IsCharArray<CharT[N]>
            : std::bool_constant<FixedStringChar<std::remove_cv_t<CharT>> && !std::is_volatile_v<CharT>> {
        };

        template <FixedStringType T>
        [[nodiscard]] constexpr auto stringData(const T& string) noexcept
        {
            if constexpr (StaticStringType<T>)
                return string.constData();
            else
                return string.borrowedData();
        }

    } // namespace detail

    template <typename T>
    concept CharArray = detail::IsCharArray<std::remove_reference_t<T>>::value;
    template <typename T>
    concept FixedStringCompatible = FixedStringType<T> || CharArray<T>;

    template <FixedStringChar CharT, std::size_t N>
    [[nodiscard]] consteval auto toFixedString(const CharT(&source)[N])
    {
        return BasicFixedString<CharT, N>{ source };
    }

    template <OwningFixedStringType T>
    [[nodiscard]] consteval auto toFixedString(const T& string)
    {
        return string;
    }

    template <detail::StringKey Key>
    [[nodiscard]] consteval auto toFixedString(const StaticString<Key>&)
    {
        return BasicFixedString<typename decltype(Key)::value_type, Key.capacity, Key.encoding>{ Key.data };
    }

    namespace detail {

        template <FixedStringChar CharT, std::size_t N>
        [[nodiscard]] consteval auto makeKey(const CharT(&source)[N])
        {
            return normalizeKey(StringKey<CharT, N>{ source });
        }

        template <FixedStringType T>
        [[nodiscard]] consteval auto makeKey(const T& string)
        {
            using String = std::remove_cvref_t<T>;
            StringKey<typename String::value_type, String::capacity, String::encoding> key{};
            for (std::size_t i = 0; i < string.size(); ++i)
                key.data[i] = stringData(string)[i];
            return normalizeKey(key);
        }

        template <StringKey Key>
        [[nodiscard]] consteval auto materialize()
        {
            return StaticString<normalizeKey(Key)>{};
        }

        template <CharArray T>
        using char_array_char_t = std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<T>>>;

        template <FixedStringChar DestinationCharT, FixedStringChar SourceCharT>
        inline constexpr bool arrayCharCompatible =
            std::same_as<DestinationCharT, SourceCharT>
            || (std::same_as<SourceCharT, char> && !std::same_as<DestinationCharT, char>);

        template <typename... Values>
        struct FirstFixedString { using type = void; };
        template <typename First, typename... Rest>
        struct FirstFixedString<First, Rest...>
        {
            using type = std::conditional_t<FixedStringType<First>,
                std::remove_cvref_t<First>, typename FirstFixedString<Rest...>::type>;
        };

        template <typename First, typename... Rest>
        consteval auto concatCharIdentity()
        {
            using String = typename FirstFixedString<First, Rest...>::type;
            if constexpr (!std::same_as<String, void>)
                return std::type_identity<typename String::value_type>{};
            else
                return std::type_identity<char_array_char_t<First>>{};
        }

        template <typename First, typename... Rest>
        using concat_char_t = typename decltype(concatCharIdentity<First, Rest...>())::type;

        template <typename First, typename... Rest>
        consteval Encoding concatEncoding()
        {
            using String = typename FirstFixedString<First, Rest...>::type;
            if constexpr (!std::same_as<String, void>)
                return String::encoding;
            else
                return textEncoding<concat_char_t<First, Rest...>>;
        }

        template <FixedStringChar CharT, Encoding Kind, bool AllowOrdinaryUtf8, typename Value>
        consteval bool operandCompatible()
        {
            if constexpr (FixedStringType<Value>) {
                return std::same_as<CharT, fixed_string_char_t<Value>>
                    && Kind == std::remove_cvref_t<Value>::encoding;
            }
            else if constexpr (CharArray<Value>) {
                if constexpr (AllowOrdinaryUtf8)
                    return arrayCharCompatible<CharT, char_array_char_t<Value>>;
                else
                    return std::same_as<CharT, char_array_char_t<Value>>;
            }
            else {
                return false;
            }
        }

        template <typename First, typename... Rest>
        consteval bool concatOperandsCompatible()
        {
            if constexpr (!(FixedStringCompatible<First> && (FixedStringCompatible<Rest> && ...))) {
                return false;
            }
            else {
                using CharT = concat_char_t<First, Rest...>;
                constexpr auto kind = concatEncoding<First, Rest...>();
                constexpr bool allowOrdinaryUtf8 = FixedStringType<First> || (FixedStringType<Rest> || ...);
                return operandCompatible<CharT, kind, allowOrdinaryUtf8, First>()
                    && (operandCompatible<CharT, kind, allowOrdinaryUtf8, Rest>() && ...);
            }
        }

        template <FixedStringCompatible T>
        consteval std::size_t operandCapacity()
        {
            if constexpr (FixedStringType<T>)
                return std::remove_cvref_t<T>::capacity;
            else
                return std::extent_v<std::remove_reference_t<T>>;
        }

        template <std::size_t... Capacities>
        consteval std::size_t concatenatedCapacity()
        {
            static_assert(((Capacities > 0) && ...));
            std::size_t result = 1;
            for (const auto capacity : { Capacities... }) {
                if (capacity - 1 > std::numeric_limits<std::size_t>::max() - result)
                    throw "fixed_string: concatenation capacity overflow";
                result += capacity - 1;
            }
            return result;
        }

        template <FixedStringChar CharT, Encoding Kind, FixedStringCompatible T>
        consteval auto normalizeOperand(const T& value)
        {
            if constexpr (FixedStringType<T>)
                return toFixedString(value);
            else
                return BasicFixedString<CharT, operandCapacity<T>(), Kind>{ value };
        }

        // Each operand is normalized once and appended once. No growing prefix fold.
        template <FixedStringChar CharT, Encoding Kind, FixedStringCompatible... Values>
        consteval auto buildConcat(const Values&... values)
        {
            constexpr auto capacity = concatenatedCapacity<operandCapacity<Values>()...>();
            CharT result[capacity]{};
            std::size_t position = 0;
            const auto append = [&](const auto& normalized) constexpr {
                for (std::size_t i = 0; i < normalized.size(); ++i)
                    result[position++] = normalized.borrowedData()[i];
                };
            (append(normalizeOperand<CharT, Kind>(values)), ...);
            return BasicFixedString<CharT, capacity, Kind>{ result };
        }

    } // namespace detail

    namespace literals {

        template <detail::StringKey Key>
        [[nodiscard]] consteval auto operator""_fs()
        {
            return detail::materialize<Key>();
        }

        template <detail::StringKey Key>
            requires (Key.encoding != Encoding::Bytes)
        [[nodiscard]] consteval auto operator""_fs8()
        {
            constexpr auto utf8 =
                detail::transcodeKey<
                Key,
                detail::TranscodeTarget::Utf8Char8>();

            return detail::materialize<utf8>();
        }

        template <detail::StringKey Key>
            requires (Key.encoding != Encoding::Bytes)
        [[nodiscard]] consteval auto operator""_fs16()
        {
            constexpr auto utf16 =
                detail::transcodeKey<
                Key,
                detail::TranscodeTarget::Utf16>();

            return detail::materialize<utf16>();
        }

        template <detail::StringKey Key>
            requires std::same_as<typename decltype(Key)::value_type, char>
        [[nodiscard]] consteval auto operator""_fbytes()
        {
            constexpr detail::StringKey<char, Key.capacity, Encoding::Bytes> bytes{ Key.data };
            return detail::materialize<bytes>();
        }

    } // namespace literals

    template <FixedStringCompatible First, FixedStringCompatible... Rest>
        requires (detail::concatOperandsCompatible<First, Rest...>())
    [[nodiscard]] consteval auto concat(const First& first, const Rest&... rest)
    {
        using CharT = detail::concat_char_t<First, Rest...>;
        constexpr auto kind = detail::concatEncoding<First, Rest...>();
        if constexpr (StaticStringType<First> && (StaticStringType<Rest> && ...)) {
            // Types carry the contents, so this does not materialize function parameters.
            constexpr auto owned = detail::buildConcat<CharT, kind>(First{}, Rest{}...);
            return detail::materialize<detail::makeKey(owned)>();
        }
        else {
            return detail::buildConcat<CharT, kind>(first, rest...);
        }
    }

    template <FixedStringCompatible Left, FixedStringCompatible Right>
        requires ((FixedStringType<Left> || FixedStringType<Right>)
    && detail::concatOperandsCompatible<Left, Right>())
        [[nodiscard]] consteval auto operator+(const Left& lhs, const Right& rhs)
    {
        return concat(lhs, rhs);
    }

} // namespace fixed_string
#endif 