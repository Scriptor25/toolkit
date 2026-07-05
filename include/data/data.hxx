#pragma once

#include <variant>

namespace data
{
    using undefined_type = std::monostate;
    using integer_type = long long int;
    using floating_point_type = long double;

    template<typename...>
    struct node_traits;

    template<typename...>
    class node_base;

    template<typename N, typename T>
    bool from_data_fn(const N &node, T &value);

    template<typename N, typename T>
    void to_data_fn(N &node, T &&value);
}
