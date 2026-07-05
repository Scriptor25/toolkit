#pragma once

#include <toolkit/result.hxx>

#include <charconv>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace toolkit
{
    template<typename S>
    void split(
        std::vector<S> &dst,
        std::basic_string_view<typename S::value_type> src,
        std::basic_string_view<typename S::value_type> delim)
    {
        dst.clear();

        size_t b{}, e{};
        for (; (e = src.find(delim, b)) != std::basic_string_view<typename S::value_type>::npos; b = e + delim.size())
            if (b != e)
                dst.emplace_back(src.substr(b, e - b));

        if (b != e)
            dst.emplace_back(src.substr(b, e - b));
    }

    template<typename S>
    void split(
        std::vector<S> &dst,
        std::basic_string_view<typename S::value_type> src,
        typename S::value_type delim)
    {
        dst.clear();

        size_t b{}, e{};
        for (; (e = src.find(delim, b)) != std::basic_string_view<typename S::value_type>::npos; b = e + 1)
            if (b != e)
                dst.emplace_back(src.substr(b, e - b));

        if (b != e)
            dst.emplace_back(src.substr(b, e - b));
    }

    template<typename S>
    std::vector<S> split(const S &src, typename S::value_type delim)
    {
        std::vector<S> dst;
        split(dst, src, delim);
        return dst;
    }

    template<typename S>
    std::vector<S> split(const S &src, std::basic_string_view<typename S::value_type> delim)
    {
        std::vector<S> dst;
        split(dst, src, delim);
        return dst;
    }

    template<typename S>
    void join(
        std::basic_string<typename S::value_type> &dst,
        std::span<const S> src,
        typename S::value_type delim)
    {
        dst.clear();

        for (auto it = src.begin(); it != src.end(); ++it)
        {
            if (it != src.begin())
                dst += delim;

            dst += *it;
        }
    }

    template<typename S>
    void join(
        std::basic_string<typename S::value_type> &dst,
        std::span<const S> src,
        std::basic_string_view<typename S::value_type> delim)
    {
        dst.clear();

        for (auto it = src.begin(); it != src.end(); ++it)
        {
            if (it != src.begin())
                dst += delim;

            dst += *it;
        }
    }

    template<typename S>
    std::basic_string<typename S::value_type> join(
        std::span<const S> src,
        typename S::value_type delim)
    {
        std::basic_string<typename S::value_type> dst;
        join(dst, src, delim);
        return dst;
    }

    template<typename S>
    std::basic_string<typename S::value_type> join(
        std::span<const S> src,
        std::basic_string_view<typename S::value_type> delim)
    {
        std::basic_string<typename S::value_type> dst;
        join(dst, src, delim);
        return dst;
    }

    template<typename S>
    void trim(std::basic_string<typename S::value_type> &dst, const S &src)
    {
        using I = S::const_iterator;

        I begin, end;

        for (auto it = src.begin(); it != src.end(); ++it)
            if (*it > 0x20)
            {
                begin = it;
                break;
            }

        for (auto it = src.rbegin(); it != src.rend(); ++it)
            if (*it > 0x20)
            {
                end = it.base();
                break;
            }

        dst = { begin, end };
    }

    template<typename S>
    std::basic_string<typename S::value_type> trim(const S &src)
    {
        std::basic_string<typename S::value_type> dst;
        trim(dst, src);
        return dst;
    }

    template<typename S>
    std::basic_string<typename S::value_type> lowercase(const S &src)
    {
        std::basic_string<typename S::value_type> dst(src.size(), 0);
        for (size_t i = 0; i < src.size(); ++i)
            dst[i] = std::tolower(src[i]);
        return dst;
    }

    template<typename S>
    std::basic_string<typename S::value_type> uppercase(const S &src)
    {
        std::basic_string<typename S::value_type> dst(src.size(), 0);
        for (size_t i = 0; i < src.size(); ++i)
            dst[i] = std::toupper(src[i]);
        return dst;
    }

    std::istream &get_line(std::istream &stream, std::string &string, std::string_view delimiter);

    template<std::integral T>
    [[nodiscard]] result<T> parse_string(const std::string &str, int base = 10)
    {
        T value;
        auto [_, ec] = std::from_chars(str.data(), str.data() + str.size(), value, base);
        if (ec != std::errc())
            return toolkit::make_error("failed to parse '{}': {}", str, ec);
        return value;
    }

    template<std::floating_point T>
    [[nodiscard]] result<T> ParseString(const std::string &str, std::chars_format fmt = std::chars_format::general)
    {
        T value;
        auto [_, ec] = std::from_chars(str.data(), str.data() + str.size(), value, fmt);
        if (ec != std::errc())
            return toolkit::make_error("failed to parse '{}': {}", str, ec);
        return value;
    }

    template<typename C>
    auto write_format_string(C &&ctx, const std::string_view str)
    {
        for (auto c : str)
            *ctx.out()++ = c;
        return ctx.out();
    }

    extern const std::unordered_map<std::errc, const char *> error_strings_map;
}

template<>
struct std::formatter<std::errc>
{
    template<typename C>
    constexpr auto parse(C &&ctx)
    {
        return ctx.begin();
    }

    template<typename C>
    auto format(const std::errc &value, C &&ctx) const
    {
        if (const auto it = toolkit::error_strings_map.find(value); it != toolkit::error_strings_map.end())
        {
            return toolkit::write_format_string(ctx, it->second);
        }

        return toolkit::write_format_string(ctx, "undefined");
    }
};
