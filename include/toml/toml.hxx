#pragma once

#include <data/node.hxx>

#include <cstdint>
#include <optional>
#include <string>

namespace toml
{
    using undefined_t = data::undefined_t;
    using boolean_t = bool;
    using integer_t = data::integer_t;
    using floating_point_t = data::floating_point_t;
    using string_t = std::string;

    struct local_date_t
    {
        uint32_t year{};
        uint32_t month{};
        uint32_t day{};
    };

    struct local_time_t
    {
        uint32_t hour{};
        uint32_t minute{};
        uint32_t second{};

        long double fraction{};
    };

    struct date_time_t
    {
        struct time_offset_t
        {
            uint32_t hours{};
            uint32_t minutes{};
        };

        local_date_t date;
        local_time_t time;

        std::optional<time_offset_t> offset;
    };

    using node_t = data::node_t<
        boolean_t,
        integer_t,
        floating_point_t,
        string_t,
        local_date_t,
        local_time_t,
        date_time_t
    >;

    using array_t = node_t::vec_type;
    using table_t = node_t::map_type;
}

template<>
struct data::node_traits_t<
            toml::boolean_t,
            toml::integer_t,
            toml::floating_point_t,
            toml::string_t,
            toml::local_date_t,
            toml::local_time_t,
            toml::date_time_t
        >
{
    static std::istream &parse(std::istream &stream, toml::node_t &node);
};
