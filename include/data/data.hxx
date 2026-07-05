#pragma once

#include <variant>

namespace data
{
    using undefined_t = std::monostate;
    using integer_t = long long int;
    using floating_point_t = long double;

    template<typename...>
    struct node_traits_t;

    template<typename...>
    class node_t;

    template<typename N, typename T>
    bool from_data_fn(const N &node, T &value);

    template<typename N, typename T>
    void to_data_fn(N &node, T &&value);
}
