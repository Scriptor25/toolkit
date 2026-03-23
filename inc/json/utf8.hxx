#pragma once

#include <array>
#include <cstdint>

template<typename T>
concept is_byte = std::is_integral_v<T> && sizeof(T) == 1;

template<typename T>
concept is_byte_iterator = std::contiguous_iterator<T> && is_byte<typename std::iterator_traits<T>::value_type>;

namespace json::utf8
{
    constexpr auto len_byte(const std::uint8_t b)
    {
        return (b & 0b10000000) == 0b00000000
                   ? 1
                   : (b & 0b11000000) == 0b10000000
                   ? 0
                   : (b & 0b11100000) == 0b11000000
                   ? 2
                   : (b & 0b11110000) == 0b11100000
                   ? 3
                   : (b & 0b11111000) == 0b11110000
                   ? 4
                   : 0;
    }

    template<std::size_t... I>
    constexpr std::array<int, 256> len_table(std::index_sequence<I...>)
    {
        return { len_byte(I)... };
    }

    constexpr auto len_lookup = len_table(std::make_index_sequence<256>());

    template<is_byte_iterator I>
    char32_t decode(I &i, I e)
    {
        auto s = i;

        const std::uint32_t c0 = (*s++ & 0xFF);

        const auto len = len_lookup[c0];
        if (len == 1)
            return *i++;

        const std::uint32_t c1 = (s != e ? (*s++ & 0xFF) : 0);
        const std::uint32_t c2 = (s != e ? (*s++ & 0xFF) : 0);
        const std::uint32_t c3 = (s != e ? (*s++ & 0xFF) : 0);

        i += len;

        return (((c0 & 0x07) << 18)
                | ((c1 & 0x3F) << 12)
                | ((c2 & 0x3F) << 6)
                | (c3 & 0x3F))
               >> (4 - len) * 6;
    }

    template<is_byte T>
    auto encode(const char32_t c, T o[4])
    {
        auto len = 1 + (c > 0x7F) + (c > 0x7FF) + (c > 0xFFFF);

        o[0] = static_cast<T>(0x80 | ((c >> 18) & 0x07));
        o[1] = static_cast<T>(0x80 | ((c >> 12) & 0x3F));
        o[2] = static_cast<T>(0x80 | ((c >> 6) & 0x3F));
        o[3] = static_cast<T>(0xF0 | (c & 0x3F));

        static constexpr std::uint8_t mask[4]{ 0x7F, 0x1F, 0x0F, 0x07 };
        static constexpr std::uint8_t offset[4]{ 0x00, 0xC0, 0xE0, 0xF0 };

        o[4 - len] = static_cast<T>(offset[len - 1] | ((c >> (6 * (len - 1))) & mask[len - 1]));

        return len;
    }
}
