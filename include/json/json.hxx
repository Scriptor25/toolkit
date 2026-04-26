#pragma once

#include <data/node.hxx>

#include <string>

namespace json
{
    using Undefined = data::Undefined;
    using Null = std::nullptr_t;
    using Boolean = bool;
    using Integer = data::Integer;
    using FloatingPoint = data::FloatingPoint;
    using String = std::string;

    using Node = data::Node<
        Null,
        Boolean,
        Integer,
        FloatingPoint,
        String
    >;

    using Array = Node::Vec;
    using Object = Node::Map;
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
    static std::ostream &print(std::ostream &stream, const json::Node &node);
    static std::istream &parse(std::istream &stream, json::Node &node);
};
