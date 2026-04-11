#pragma once

#include <data/node.hxx>

#include <string>

namespace json
{
    using Null = std::nullptr_t;
    using Boolean = bool;
    using Integer = long long;
    using FloatingPoint = long double;
    using String = std::string;

    using Node = data::Node<
        Null,
        Boolean,
        Integer,
        FloatingPoint,
        String
    >;
}

template<>
struct data::NodeTraits<
            json::Null,
            json::Boolean,
            json::Integer,
            json::FloatingPoint,
            json::String
        >
{
    using Integer = json::Integer;
    using FloatingPoint = json::FloatingPoint;

    static std::ostream &print(std::ostream &stream, const json::Node &node);
    static std::istream &parse(std::istream &stream, json::Node &node);
};
