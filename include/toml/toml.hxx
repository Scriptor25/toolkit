#pragma once

#include <data/node.hxx>

#include <cstdint>
#include <optional>
#include <string>

namespace toml
{
    using undefined = data::undefined_type;
    using boolean = bool;
    using integer = data::integer_type;
    using floating_point = data::floating_point_type;
    using string = std::string;

    struct local_date
    {
        uint32_t year{};
        uint32_t month{};
        uint32_t day{};
    };

    struct local_time
    {
        uint32_t hour{};
        uint32_t minute{};
        uint32_t second{};

        long double fraction{};
    };

    struct date_time
    {
        struct time_offset
        {
            uint32_t hours{};
            uint32_t minutes{};
        };

        local_date date;
        local_time time;

        std::optional<time_offset> offset;
    };

    using node = data::node_base<
        boolean,
        integer,
        floating_point,
        string,
        local_date,
        local_time,
        date_time
    >;

    using array = node::vec_type;
    using table = node::map_type;
}

template<>
struct data::node_traits<
            toml::boolean,
            toml::integer,
            toml::floating_point,
            toml::string,
            toml::local_date,
            toml::local_time,
            toml::date_time
        >
{
    static std::istream &parse(std::istream &stream, toml::node &node);
};
