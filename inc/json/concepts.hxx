#pragma once

#include <json/forward.hxx>

#include <concepts>
#include <optional>
#include <set>

namespace json
{
    template<typename T>
    concept node = std::same_as<std::remove_cvref_t<T>, Node>;

    template<typename T>
    concept node_value = std::same_as<std::remove_cvref_t<T>, NodeValue>;

    template<typename T>
    concept primitive = std::same_as<std::remove_cvref_t<T>, Undefined>
                        || std::same_as<std::remove_cvref_t<T>, Null>
                        || std::same_as<std::remove_cvref_t<T>, Boolean>
                        || std::same_as<std::remove_cvref_t<T>, Number>
                        || std::same_as<std::remove_cvref_t<T>, String>
                        || std::same_as<std::remove_cvref_t<T>, Array>
                        || std::same_as<std::remove_cvref_t<T>, Object>;

    template<typename T>
    concept assignable = !node<T> && !node_value<T> && !primitive<T>;

    template<typename T>
    concept integral = std::integral<std::remove_cvref_t<T>> && !primitive<T>;

    template<typename T>
    concept floating_point = std::floating_point<std::remove_cvref_t<T>> && !primitive<T>;

    template<typename>
    struct is_vector : std::false_type
    {
    };

    template<typename... Args>
    struct is_vector<std::vector<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept vector = is_vector<std::remove_cvref_t<T>>::value && !primitive<T>;

    template<typename>
    struct is_set : std::false_type
    {
    };

    template<typename... Args>
    struct is_set<std::set<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept set = is_set<std::remove_cvref_t<T>>::value;

    template<typename>
    struct is_array : std::false_type
    {
    };

    template<typename T, std::size_t N>
    struct is_array<std::array<T, N>> : std::true_type
    {
    };

    template<typename T>
    concept array = is_array<std::remove_cvref_t<T>>::value;

    template<typename>
    struct is_map : std::false_type
    {
    };

    template<typename... Args>
    struct is_map<std::map<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept map = is_map<std::remove_cvref_t<T>>::value && !primitive<T>;

    template<typename>
    struct is_optional : std::false_type
    {
    };

    template<typename... Args>
    struct is_optional<std::optional<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept optional = is_optional<std::remove_cvref_t<T>>::value;

    template<typename>
    struct is_variant : std::false_type
    {
    };

    template<typename... Args>
    struct is_variant<std::variant<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept variant = is_variant<std::remove_cvref_t<T>>::value;

    template<typename>
    struct serializer
    {
    };

    template<typename T>
    concept enable_from_json = requires(Node &node, T &value)
    {
        serializer<std::decay_t<T>>::from_json(node, value);
    };

    template<typename T>
    concept enable_to_json = requires(Node &node, T &&value)
    {
        serializer<std::decay_t<T>>::to_json(node, std::forward<T>(value));
    };
}
