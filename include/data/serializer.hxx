#pragma once

#include <type_traits>
#include <utility>

namespace data
{
    template<typename>
    struct serializer
    {
    };

    template<typename N, typename T>
    concept enable_from_data = requires(const N &node, T &value)
    {
        serializer<std::decay_t<T>>::from_data(node, value);
    };

    template<typename N, typename T>
    concept enable_to_data = requires(N &node, T &&value)
    {
        serializer<std::decay_t<T>>::to_data(node, std::forward<T>(value));
    };
}

template<typename N, typename T>
bool from_data(const N &node, T &value) = delete;

template<typename N, typename T>
void to_data(N &node, T &&value) = delete;
