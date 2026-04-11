#pragma once

#include <array>
#include <concepts>
#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <variant>
#include <vector>

namespace data
{
    template<typename... V>
    struct Node;

    template<typename>
    struct is_node_t : std::false_type
    {
    };

    template<typename... V>
    struct is_node_t<Node<V...>> : std::true_type
    {
    };

    template<typename T>
    concept node = is_node_t<std::decay_t<T>>::value;

    template<typename, typename>
    struct in_variant_t : std::false_type
    {
    };

    template<typename T, typename... E>
    struct in_variant_t<T, std::variant<E...>> : std::disjunction<std::is_same<T, E>...>
    {
    };

    template<typename T, typename V>
    concept in_variant = in_variant_t<T, V>::value;

    template<typename T, typename N>
    concept decay_same_as = std::same_as<std::decay_t<T>, N>;

    template<typename T, typename N>
    concept node_value_of = std::same_as<std::decay_t<T>, typename N::ValueType>;

    template<typename T, typename N>
    concept primitive = data::in_variant<std::decay_t<T>, typename N::ValueType>;

    template<typename T, typename N, typename V>
    concept assignable = !decay_same_as<T, N> && !node_value_of<T, N> && !primitive<T, N>;

    template<typename T, typename N>
    concept integral = std::integral<std::decay_t<T>> && !primitive<T, N>;

    template<typename T, typename N>
    concept floating_point = std::floating_point<std::decay_t<T>> && !primitive<T, N>;

    template<typename>
    struct is_vector_t : std::false_type
    {
    };

    template<typename... Args>
    struct is_vector_t<std::vector<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept vector = is_vector_t<std::decay_t<T>>::value;

    template<typename>
    struct is_set_t : std::false_type
    {
    };

    template<typename... Args>
    struct is_set_t<std::set<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept set = is_set_t<std::decay_t<T>>::value;

    template<typename>
    struct is_array_t : std::false_type
    {
    };

    template<typename T, std::size_t N>
    struct is_array_t<std::array<T, N>> : std::true_type
    {
    };

    template<typename T>
    concept array = is_array_t<std::decay_t<T>>::value;

    template<typename>
    struct is_map_t : std::false_type
    {
    };

    template<typename... Args>
    struct is_map_t<std::map<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept map = is_map_t<std::decay_t<T>>::value;

    template<typename>
    struct is_optional_t : std::false_type
    {
    };

    template<typename... Args>
    struct is_optional_t<std::optional<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept optional = is_optional_t<std::decay_t<T>>::value;

    template<typename>
    struct is_variant_t : std::false_type
    {
    };

    template<typename... Args>
    struct is_variant_t<std::variant<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept variant = is_variant_t<std::decay_t<T>>::value;
}
