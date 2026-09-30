#pragma once

#include <charconv>
#include <data/node.hxx>

#include <string>

namespace xml
{
    using undefined = data::undefined_type;
    using boolean = bool;
    using string = std::string;

    using node = data::node_base<
        boolean,
        string
    >;

    using elements = node::vec_type;
    using element = node::map_type;
}

template<>
struct data::node_traits<
            xml::boolean,
            xml::string
        >
{
    static std::ostream &print(std::ostream &stream, const xml::node &node);
    static std::istream &parse(std::istream &stream, xml::node &node);
};

namespace data
{
    template<floating_point<xml::node> T>
    struct serializer<xml::node, T>
    {
        static bool from_data(const xml::node &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(&val.front(), &val.back(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::node &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };

    template<integral<xml::node> T>
    struct serializer<xml::node, T>
    {
        static bool from_data(const xml::node &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(val.begin().base(), val.end().base(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::node &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };
}
