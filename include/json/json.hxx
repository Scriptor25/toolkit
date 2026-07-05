#pragma once

#include <data/node.hxx>

#include <string>

namespace json
{
    using undefined_t = data::undefined_t;
    using null_t = std::nullptr_t;
    using boolean_t = bool;
    using integer_t = data::integer_t;
    using floating_point_t = data::floating_point_t;
    using string_t = std::string;

    using node_t = data::node_t<
        null_t,
        boolean_t,
        integer_t,
        floating_point_t,
        string_t
    >;

    using array_t = node_t::vec_type;
    using object_t = node_t::map_type;
}

template<>
struct data::node_traits_t<
            json::null_t,
            json::boolean_t,
            json::integer_t,
            json::floating_point_t,
            json::string_t
        >
{
    static std::ostream &print(std::ostream &stream, const json::node_t &node);
    static std::istream &parse(std::istream &stream, json::node_t &node);
};
