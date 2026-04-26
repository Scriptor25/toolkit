#pragma once

#include <array>
#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <variant>
#include <vector>

namespace toolkit
{
    template<typename, typename>
    struct is_in_variant : std::false_type
    {
    };

    template<typename T, typename... E>
    struct is_in_variant<T, std::variant<E...>> : std::disjunction<std::is_same<T, E>...>
    {
    };

    template<typename T, typename V>
    concept in_variant = is_in_variant<T, V>::value;

    template<typename T, typename U>
    concept same_as = std::same_as<std::decay_t<T>, std::decay_t<U>>;

    template<typename>
    struct is_vector : std::false_type
    {
    };

    template<typename... Args>
    struct is_vector<std::vector<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept vector_type = is_vector<std::decay_t<T>>::value;

    template<typename>
    struct is_set : std::false_type
    {
    };

    template<typename... Args>
    struct is_set<std::set<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept set_type = is_set<std::decay_t<T>>::value;

    template<typename>
    struct is_array : std::false_type
    {
    };

    template<typename T, std::size_t N>
    struct is_array<std::array<T, N>> : std::true_type
    {
    };

    template<typename T>
    concept array_type = is_array<std::decay_t<T>>::value;

    template<typename>
    struct is_map : std::false_type
    {
    };

    template<typename... Args>
    struct is_map<std::map<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept map_type = is_map<std::decay_t<T>>::value;

    template<typename>
    struct is_optional : std::false_type
    {
    };

    template<typename... Args>
    struct is_optional<std::optional<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept optional_type = is_optional<std::decay_t<T>>::value;

    template<typename>
    struct is_variant : std::false_type
    {
    };

    template<typename... Args>
    struct is_variant<std::variant<Args...>> : std::true_type
    {
    };

    template<typename T>
    concept variant_type = is_variant<std::decay_t<T>>::value;

    template<typename, typename T>
    struct copy_cv
    {
        using type = T;
    };

    template<typename F, typename T>
    struct copy_cv<const F, T>
    {
        using type = std::add_const_t<T>;
    };

    template<typename F, typename T>
    struct copy_cv<volatile F, T>
    {
        using type = std::add_volatile_t<T>;
    };

    template<typename F, typename T>
    struct copy_cv<const volatile F, T>
    {
        using type = std::add_cv_t<T>;
    };

    template<typename F, typename T>
    using copy_cv_t = copy_cv<F, T>::type;

    template<typename F, typename T>
    struct copy_cvref
    {
        using type = copy_cv_t<std::remove_reference_t<F>, T>;
    };

    template<typename F, typename T>
    struct copy_cvref<F &, T>
    {
        using type = std::add_lvalue_reference_t<copy_cv_t<std::remove_reference_t<F>, T>>;
    };

    template<typename F, typename T>
    struct copy_cvref<F &&, T>
    {
        using type = std::add_rvalue_reference_t<copy_cv_t<std::remove_reference_t<F>, T>>;
    };

    template<typename F, typename T>
    using copy_cvref_t = copy_cvref<F, T>::type;
}
