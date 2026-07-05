#pragma once

#include <data/node.hxx>

#include <string>

namespace json
{
    using undefined = data::undefined_type;
    using null = std::nullptr_t;
    using boolean = bool;
    using integer = data::integer_type;
    using floating_point = data::floating_point_type;
    using string = std::string;

    using node = data::node_base<
        null,
        boolean,
        integer,
        floating_point,
        string
    >;

    using array = node::vec_type;
    using object = node::map_type;
}

template<>
struct data::node_traits<
            json::null,
            json::boolean,
            json::integer,
            json::floating_point,
            json::string
        >
{
    static std::ostream &print(std::ostream &stream, const json::node &node);
    static std::istream &parse(std::istream &stream, json::node &node);
};
