#pragma once

#include <data/node.hxx>

#include <cstdint>
#include <optional>
#include <string>

namespace toml
{
    using Boolean = bool;
    using Integer = long long;
    using FloatingPoint = long double;
    using String = std::string;

    struct LocalDate
    {
        uint32_t Year{};
        uint32_t Month{};
        uint32_t Day{};
    };

    struct LocalTime
    {
        uint32_t Hour{};
        uint32_t Minute{};
        uint32_t Second{};

        long double Fraction{};
    };

    struct DateTime
    {
        struct TimeOffset
        {
            uint32_t Hours{};
            uint32_t Minutes{};
        };

        LocalDate Date;
        LocalTime Time;

        std::optional<TimeOffset> Offset;
    };

    using Node = data::Node<
        Boolean,
        Integer,
        FloatingPoint,
        String,
        LocalDate,
        LocalTime,
        DateTime
    >;
}

template<>
struct data::NodeTraits<
            toml::Boolean,
            toml::Integer,
            toml::FloatingPoint,
            toml::String,
            toml::LocalDate,
            toml::LocalTime,
            toml::DateTime
        >
{
    using Integer = toml::Integer;
    using FloatingPoint = toml::FloatingPoint;

    static std::istream &parse(std::istream &stream, toml::Node &node);
};
