#pragma once

#include <data/serializer.hxx>

#include <json/json.hxx>
#include <toml/toml.hxx>

template<>
struct data::serializer_t<toml::local_date_t>
{
    static bool from_data(const json::node_t &node, toml::local_date_t &value)
    {
        if (!node.is<json::object_t>())
            return false;

        auto ok = true;

        ok &= node["year"] >> value.year;
        ok &= node["month"] >> value.month;
        ok &= node["day"] >> value.day;

        return ok;
    }

    template<toolkit::same_as<toml::local_date_t> T>
    static void to_data(json::node_t &node, T &&value)
    {
        node = json::object_t
        {
            { "year", value.year },
            { "month", value.month },
            { "day", value.day },
        };
    }
};

template<>
struct data::serializer_t<toml::local_time_t>
{
    static bool from_data(const json::node_t &node, toml::local_time_t &value)
    {
        if (!node.is<json::object_t>())
            return false;

        auto ok = true;

        ok &= node["hour"] >> value.hour;
        ok &= node["minute"] >> value.minute;
        ok &= node["second"] >> value.second;
        ok &= node["fraction"] >> value.fraction;

        return ok;
    }

    template<toolkit::same_as<toml::local_time_t> T>
    static void to_data(json::node_t &node, T &&value)
    {
        node = json::object_t
        {
            { "hour", value.hour },
            { "minute", value.minute },
            { "second", value.second },
            { "fraction", value.fraction },
        };
    }
};

template<>
struct data::serializer_t<toml::date_time_t::time_offset_t>
{
    static bool from_data(const json::node_t &node, toml::date_time_t::time_offset_t &value)
    {
        if (!node.is<json::object_t>())
            return false;

        auto ok = true;

        ok &= node["hours"] >> value.hours;
        ok &= node["minutes"] >> value.minutes;

        return ok;
    }

    template<toolkit::same_as<toml::date_time_t::time_offset_t> T>
    static void to_data(json::node_t &node, T &&value)
    {
        node = json::object_t
        {
            { "hours", value.hours },
            { "minutes", value.minutes },
        };
    }
};

template<>
struct data::serializer_t<toml::date_time_t>
{
    static bool from_data(const json::node_t &node, toml::date_time_t &value)
    {
        if (!node.is<json::object_t>())
            return false;

        auto ok = true;

        ok &= node["date"] >> value.date;
        ok &= node["time"] >> value.time;
        ok &= node["offset"] >> value.offset;

        return ok;
    }

    template<toolkit::same_as<toml::date_time_t> T>
    static void to_data(json::node_t &node, T &&value)
    {
        node = json::object_t
        {
            { "date", value.date },
            { "time", value.time },
            { "offset", value.offset },
        };
    }
};

template<>
struct data::serializer_t<toml::node_t>
{
    static bool from_data(const json::node_t &node, toml::node_t &value)
    {
        return node >> *value;
    }

    template<toolkit::same_as<toml::node_t> T>
    static void to_data(json::node_t &node, T &&value)
    {
        node = *value;
    }
};

template<>
struct data::serializer_t<json::null_t>
{
    static bool from_data(const toml::node_t &node, json::null_t &value)
    {
        if (!node)
        {
            value = nullptr;
            return true;
        }
        return false;
    }

    template<toolkit::same_as<json::null_t> T>
    static void to_data(toml::node_t &node, T &&)
    {
        node = toml::undefined_t();
    }
};

template<>
struct data::serializer_t<json::node_t>
{
    static bool from_data(const toml::node_t &node, json::node_t &value)
    {
        return node >> *value;
    }

    template<toolkit::same_as<json::node_t> T>
    static void to_data(toml::node_t &node, T &&value)
    {
        node = *value;
    }
};
