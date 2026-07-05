#pragma once

#include <data/serializer.hxx>

#include <json/json.hxx>
#include <toml/toml.hxx>

template<>
struct data::serializer<toml::local_date>
{
    static bool from_data(const json::node &node, toml::local_date &value)
    {
        if (!node.is<json::object>())
            return false;

        auto ok = true;

        ok &= node["year"] >> value.year;
        ok &= node["month"] >> value.month;
        ok &= node["day"] >> value.day;

        return ok;
    }

    template<toolkit::same_as<toml::local_date> T>
    static void to_data(json::node &node, T &&value)
    {
        node = json::object
        {
            { "year", value.year },
            { "month", value.month },
            { "day", value.day },
        };
    }
};

template<>
struct data::serializer<toml::local_time>
{
    static bool from_data(const json::node &node, toml::local_time &value)
    {
        if (!node.is<json::object>())
            return false;

        auto ok = true;

        ok &= node["hour"] >> value.hour;
        ok &= node["minute"] >> value.minute;
        ok &= node["second"] >> value.second;
        ok &= node["fraction"] >> value.fraction;

        return ok;
    }

    template<toolkit::same_as<toml::local_time> T>
    static void to_data(json::node &node, T &&value)
    {
        node = json::object
        {
            { "hour", value.hour },
            { "minute", value.minute },
            { "second", value.second },
            { "fraction", value.fraction },
        };
    }
};

template<>
struct data::serializer<toml::date_time::time_offset>
{
    static bool from_data(const json::node &node, toml::date_time::time_offset &value)
    {
        if (!node.is<json::object>())
            return false;

        auto ok = true;

        ok &= node["hours"] >> value.hours;
        ok &= node["minutes"] >> value.minutes;

        return ok;
    }

    template<toolkit::same_as<toml::date_time::time_offset> T>
    static void to_data(json::node &node, T &&value)
    {
        node = json::object
        {
            { "hours", value.hours },
            { "minutes", value.minutes },
        };
    }
};

template<>
struct data::serializer<toml::date_time>
{
    static bool from_data(const json::node &node, toml::date_time &value)
    {
        if (!node.is<json::object>())
            return false;

        auto ok = true;

        ok &= node["date"] >> value.date;
        ok &= node["time"] >> value.time;
        ok &= node["offset"] >> value.offset;

        return ok;
    }

    template<toolkit::same_as<toml::date_time> T>
    static void to_data(json::node &node, T &&value)
    {
        node = json::object
        {
            { "date", value.date },
            { "time", value.time },
            { "offset", value.offset },
        };
    }
};

template<>
struct data::serializer<toml::node>
{
    static bool from_data(const json::node &node, toml::node &value)
    {
        return node >> *value;
    }

    template<toolkit::same_as<toml::node> T>
    static void to_data(json::node &node, T &&value)
    {
        node = *value;
    }
};

template<>
struct data::serializer<json::null>
{
    static bool from_data(const toml::node &node, json::null &value)
    {
        if (!node)
        {
            value = nullptr;
            return true;
        }
        return false;
    }

    template<toolkit::same_as<json::null> T>
    static void to_data(toml::node &node, T &&)
    {
        node = toml::undefined();
    }
};

template<>
struct data::serializer<json::node>
{
    static bool from_data(const toml::node &node, json::node &value)
    {
        return node >> *value;
    }

    template<toolkit::same_as<json::node> T>
    static void to_data(toml::node &node, T &&value)
    {
        node = *value;
    }
};
