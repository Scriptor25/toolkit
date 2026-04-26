#pragma once

#include <string>
#include <variant>

namespace data
{
    using Index = unsigned long long;
    using Key = std::string;

    using Undefined = std::monostate;
    using Integer = long long int;
    using FloatingPoint = long double;

    template<typename...>
    struct NodeTraits;

    template<typename...>
    struct Node;

    template<typename N, typename T>
    bool from_data_fn(const N &node, T &value);

    template<typename N, typename T>
    void to_data_fn(N &node, T &&value);
}
