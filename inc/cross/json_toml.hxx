#pragma once

#include <data/serializer.hxx>
#include <json/json.hxx>
#include <toml/toml.hxx>

template<>
struct data::serializer<toml::LocalDate>
{
    static bool from_data(const json::Node &node, toml::LocalDate &value)
    {
        if (!node.Is<json::Node::Map>())
            return false;

        auto ok = true;

        ok &= node["year"] >> value.Year;
        ok &= node["month"] >> value.Month;
        ok &= node["day"] >> value.Day;

        return ok;
    }

    template<decay_same_as<toml::LocalDate> T>
    static void to_data(json::Node &node, T &&value)
    {
        node = json::Node::Map
        {
            { "year", value.Year },
            { "month", value.Month },
            { "day", value.Day },
        };
    }
};

template<>
struct data::serializer<toml::LocalTime>
{
    static bool from_data(const json::Node &node, toml::LocalTime &value)
    {
        if (!node.Is<json::Node::Map>())
            return false;

        auto ok = true;

        ok &= node["hour"] >> value.Hour;
        ok &= node["minute"] >> value.Minute;
        ok &= node["second"] >> value.Second;
        ok &= node["fraction"] >> value.Fraction;

        return ok;
    }

    template<decay_same_as<toml::LocalTime> T>
    static void to_data(json::Node &node, T &&value)
    {
        node = json::Node::Map
        {
            { "hour", value.Hour },
            { "minute", value.Minute },
            { "second", value.Second },
            { "fraction", value.Fraction },
        };
    }
};

template<>
struct data::serializer<toml::DateTime::TimeOffset>
{
    static bool from_data(const json::Node &node, toml::DateTime::TimeOffset &value)
    {
        if (!node.Is<json::Node::Map>())
            return false;

        auto ok = true;

        ok &= node["hours"] >> value.Hours;
        ok &= node["minutes"] >> value.Minutes;

        return ok;
    }

    template<decay_same_as<toml::DateTime::TimeOffset> T>
    static void to_data(json::Node &node, T &&value)
    {
        node = json::Node::Map
        {
            { "hours", value.Hours },
            { "minutes", value.Minutes },
        };
    }
};

template<>
struct data::serializer<toml::DateTime>
{
    static bool from_data(const json::Node &node, toml::DateTime &value)
    {
        if (!node.Is<json::Node::Map>())
            return false;

        auto ok = true;

        ok &= node["date"] >> value.Date;
        ok &= node["time"] >> value.Time;
        ok &= node["offset"] >> value.Offset;

        return ok;
    }

    template<decay_same_as<toml::DateTime> T>
    static void to_data(json::Node &node, T &&value)
    {
        node = json::Node::Map
        {
            { "date", value.Date },
            { "time", value.Time },
            { "offset", value.Offset },
        };
    }
};

template<>
struct data::serializer<toml::Node>
{
    static bool from_data(const json::Node &node, toml::Node &value)
    {
        return node >> value.Value;
    }

    template<decay_same_as<toml::Node> T>
    static void to_data(json::Node &node, T &&value)
    {
        node = value.Value;
    }
};

template<>
struct data::serializer<std::nullptr_t>
{
    static bool from_data(const toml::Node &node, std::nullptr_t &value)
    {
        if (!node)
        {
            value = nullptr;
            return true;
        }
        return false;
    }

    template<decay_same_as<std::nullptr_t> T>
    static void to_data(toml::Node &node, T &&)
    {
        node = toml::Node::Undefined();
    }
};

template<>
struct data::serializer<json::Node>
{
    static bool from_data(const toml::Node &node, json::Node &value)
    {
        return node >> value.Value;
    }

    template<decay_same_as<json::Node> T>
    static void to_data(toml::Node &node, T &&value)
    {
        node = value.Value;
    }
};
