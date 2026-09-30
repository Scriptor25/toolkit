#pragma once

#include <data/data.hxx>
#include <data/serializer.hxx>

#include <toolkit/templates.hxx>

#include <stdexcept>
#include <utility>

namespace data
{
    template<typename... V>
    class node_base;

    template<typename>
    struct is_node : std::false_type
    {
    };

    template<typename... V>
    struct is_node<node_base<V...>> : std::true_type
    {
    };

    template<typename T>
    concept node_type = is_node<std::decay_t<T>>::value;

    template<typename T, typename N>
    concept node_value_of = toolkit::same_as<T, typename N::value_type>;

    template<typename T, typename N>
    concept primitive = toolkit::in_variant<std::decay_t<T>, typename N::value_type>;

    template<typename T, typename N, typename V>
    concept assignable = !toolkit::same_as<T, N> && !node_value_of<T, N> && !primitive<T, N>;

    template<typename T, typename N>
    concept integral = std::integral<std::decay_t<T>> && !primitive<T, N>;

    template<typename T, typename N>
    concept floating_point = std::floating_point<std::decay_t<T>> && !primitive<T, N>;

    template<typename... V>
    class node_base
    {
    public:
        using traits = node_traits<V...>;

        using vec_type = std::vector<node_base>;
        using map_type = std::map<std::string, node_base>;

        using value_type = std::variant<undefined_type, V..., vec_type, map_type>;

        template<typename T, typename VI, typename MI>
        struct iterator_base
        {
            using value_type = std::pair<std::string, T &>;

            explicit iterator_base(VI &&it)
                : it(std::forward<VI>(it))
            {
            }

            explicit iterator_base(MI &&it)
                : it(std::forward<MI>(it))
            {
            }

            auto operator!=(iterator_base other) const
            {
                return it != other.it;
            }

            auto operator*() const
            {
                struct
                {
                    auto operator()(const VI &i) -> value_type
                    {
                        return { {}, *i };
                    }

                    auto operator()(const MI &i) -> value_type
                    {
                        return { i->first, i->second };
                    }
                } visitor;

                return std::visit(visitor, it);
            }

            auto &&operator++()
            {
                std::visit(
                    [](auto &it)
                    {
                        ++it;
                    },
                    it);

                return *this;
            }

            std::variant<VI, MI> it{};
        };

        using iterator = iterator_base<
            node_base,
            typename vec_type::iterator,
            typename map_type::iterator>;
        using const_iterator = iterator_base<
            const node_base,
            typename vec_type::const_iterator,
            typename map_type::const_iterator>;

        node_base()
            : m_Value()
        {
        }

        node_base(const node_base &other) = default;
        node_base &operator=(const node_base &other) = default;

        node_base(node_base &&other) noexcept = default;
        node_base &operator=(node_base &&other) noexcept = default;

        explicit node_base(const value_type &value)
            : m_Value(value)
        {
        }

        node_base &operator=(const value_type &value)
        {
            m_Value = value;
            return *this;
        }

        explicit node_base(value_type &&value)
            : m_Value(std::forward<value_type>(value))
        {
        }

        node_base &operator=(value_type &&value)
        {
            m_Value = value;
            return *this;
        }

        template<primitive<node_base> T>
        node_base(T &&value)
            : m_Value(std::forward<T>(value))
        {
        }

        template<primitive<node_base> T>
        node_base &operator=(T &&value)
        {
            m_Value = std::forward<T>(value);
            return *this;
        }

        template<assignable<node_base, value_type> T>
        node_base(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
        }

        template<assignable<node_base, value_type> T>
        node_base &operator=(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
            return *this;
        }

        template<typename T>
        bool operator>>(T &value) const
        {
            return from_data_fn(*this, value);
        }

        template<primitive<node_base> T>
        [[nodiscard]] auto is() const
        {
            return std::holds_alternative<T>(m_Value);
        }

        template<primitive<node_base> T>
        auto &&get()
        {
            return std::get<T>(m_Value);
        }

        template<primitive<node_base> T>
        auto &&get() const
        {
            return std::get<T>(m_Value);
        }

        explicit operator bool() const
        {
            return !is<undefined_type>();
        }

        bool operator!() const
        {
            return is<undefined_type>();
        }

        auto &&operator*() &&
        {
            return m_Value;
        }

        auto &operator*() &
        {
            return m_Value;
        }

        const auto &operator*() const &
        {
            return m_Value;
        }

        iterator begin()
        {
            return std::visit(
                []<typename T>(T &value) -> iterator
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return iterator(value.begin());
                    else
                        throw std::runtime_error("type does not have `begin()`");
                },
                m_Value);
        }

        iterator end()
        {
            return std::visit(
                []<typename T>(T &value) -> iterator
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return iterator(value.end());
                    else
                        throw std::runtime_error("type does not have `end()`");
                },
                m_Value);
        }

        [[nodiscard]] const_iterator begin() const
        {
            return std::visit(
                []<typename T>(T &value) -> const_iterator
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return const_iterator(value.begin());
                    else
                        throw std::runtime_error("type does not have `begin() const`");
                },
                m_Value);
        }

        [[nodiscard]] const_iterator end() const
        {
            return std::visit(
                []<typename T>(T &value) -> const_iterator
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return const_iterator(value.end());
                    else
                        throw std::runtime_error("type does not have `end() const`");
                },
                m_Value);
        }

        [[nodiscard]] bool empty() const
        {
            return std::visit(
                []<typename T>(T &value) -> size_t
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return value.empty();
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return true;
                    else
                        throw std::runtime_error("type does not have `empty() const`");
                },
                m_Value);
        }

        [[nodiscard]] size_t size() const
        {
            return std::visit(
                []<typename T>(T &value) -> size_t
                {
                    if constexpr (toolkit::same_as<T, vec_type> || toolkit::same_as<T, map_type>)
                        return value.size();
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return 0;
                    else
                        throw std::runtime_error("type does not have `size() const`");
                },
                m_Value);
        }

        node_base &operator[](size_t index)
        {
            static node_base undefined;

            return std::visit(
                [&index]<typename T>(T &value) -> node_base &
                {
                    if constexpr (toolkit::same_as<T, vec_type>)
                    {
                        if (index >= value.size())
                            value.resize(index + 1);
                        return value[index];
                    }
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](size_t)`");
                },
                m_Value);
        }

        const node_base &operator[](size_t index) const
        {
            static const node_base undefined;

            return std::visit(
                [&index]<typename T>(T &value) -> const node_base &
                {
                    if constexpr (toolkit::same_as<T, vec_type>)
                        return index < value.size() ? value[index] : undefined;
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](size_t) const`");
                },
                m_Value);
        }

        node_base &operator[](const std::string &key)
        {
            static node_base undefined;

            return std::visit(
                [&key]<typename T>(T &value) -> node_base &
                {
                    if constexpr (toolkit::same_as<T, map_type>)
                        return value[key];
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Key)`");
                },
                m_Value);
        }

        const node_base &operator[](const std::string &key) const
        {
            static const node_base undefined;

            return std::visit(
                [&key]<typename T>(T &value) -> const node_base &
                {
                    if constexpr (toolkit::same_as<T, map_type>)
                        return value.contains(key) ? value.at(key) : undefined;
                    else if constexpr (toolkit::same_as<T, undefined_type>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Key) const`");
                },
                m_Value);
        }

    private:
        value_type m_Value;
    };

    template<typename... V>
    std::ostream &operator<<(std::ostream &stream, const node_base<V...> &node)
    {
        using N = node_base<V...>;
        using T = N::traits;

        return T::print(stream, node);
    }

    template<typename... V>
    std::istream &operator>>(std::istream &stream, node_base<V...> &node)
    {
        using N = node_base<V...>;
        using T = N::traits;

        return T::parse(stream, node);
    }

    template<node_type N>
    struct serializer<N, N>
    {
        static bool from_data(const N &node, N &value)
        {
            value = node;
            return true;
        }

        template<toolkit::same_as<N> U>
        static void to_data(N &node, U &&value)
        {
            node = std::forward<U>(value);
        }
    };

    template<node_type N, primitive<N> T>
    struct serializer<N, T>
    {
        static bool from_data(const N &node, T &value)
        {
            if (node.template is<T>())
            {
                value = node.template get<T>();
                return true;
            }

            return false;
        }

        static void to_data(N &node, T &&value)
        {
            node = N(std::forward<T>(value));
        }
    };

    template<node_type N, floating_point<N> T>
    struct serializer<N, T>
    {
        static bool from_data(const N &node, T &value)
        {
            if (floating_point_type val; node >> val)
            {
                value = static_cast<T>(val);
                return true;
            }

            return false;
        }

        static void to_data(N &node, T &&value)
        {
            node = static_cast<floating_point_type>(std::forward<T>(value));
        }
    };

    template<node_type N, integral<N> T>
    struct serializer<N, T>
    {
        static bool from_data(const N &node, T &value)
        {
            if (integer_type val; node >> val)
            {
                value = static_cast<T>(val);
                return true;
            }

            return false;
        }

        static void to_data(N &node, T &&value)
        {
            node = static_cast<integer_type>(std::forward<T>(value));
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::vector<T>>
    {
        static bool from_data(const N &node, std::vector<T> &value)
        {
            using vec_type = N::vec_type;

            if (!node.template is<vec_type>())
                return false;

            value.resize(node.size());

            auto ok = true;
            for (std::size_t i = 0; i < node.size(); ++i)
                ok &= node[i] >> value[i];

            return ok;
        }

        template<toolkit::same_as<std::vector<T>> U>
        static void to_data(N &node, U &&value)
        {
            using vec_type = N::vec_type;

            node = vec_type(value.size());

            for (std::size_t i = 0; i < value.size(); ++i)
                node[i] = value[i];
        }
    };

    template<node_type N, typename T, std::size_t S>
    struct serializer<N, std::array<T, S>>
    {
        static bool from_data(const N &node, std::array<T, S> &value)
        {
            using vec_type = N::vec_type;

            if (!node.template is<vec_type>() || node.size() != S)
                return false;

            auto ok = true;
            for (std::size_t i = 0; i < S; ++i)
                ok &= node[i] >> value[i];

            return ok;
        }

        template<toolkit::same_as<std::array<T, S>> U>
        static void to_data(N &node, U &&value)
        {
            using vec_type = N::vec_type;

            node = vec_type(S);

            for (std::size_t i = 0; i < S; ++i)
                node[i] = value[i];
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::set<T>>
    {
        static bool from_data(const N &node, std::set<T> &value)
        {
            if (std::vector<T> val; node >> val)
            {
                value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
                return true;
            }

            return false;
        }

        template<toolkit::same_as<std::set<T>> U>
        static void to_data(N &node, U &&value)
        {
            node = std::vector(value.begin(), value.end());
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::unordered_set<T>>
    {
        static bool from_data(const N &node, std::unordered_set<T> &value)
        {
            using value_type = T::value_type;

            if (std::vector<value_type> val; node >> val)
            {
                value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
                return true;
            }

            return false;
        }

        template<toolkit::same_as<std::unordered_set<T>> U>
        static void to_data(N &node, U &&value)
        {
            node = std::vector(value.begin(), value.end());
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::map<std::string, T>>
    {
        static bool from_data(const N &node, std::map<std::string, T> &value)
        {
            using map_type = N::map_type;

            if (!node.template is<map_type>())
                return false;

            auto ok = true;
            for (auto &&[key, val] : node)
                ok &= val >> value[key];

            return ok;
        }

        template<toolkit::same_as<std::map<std::string, T>> U>
        static void to_data(N &node, U &&value)
        {
            using map_type = N::map_type;

            node = map_type();

            for (auto &&[key, val] : value)
                node[key] = val;
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::unordered_map<std::string, T>>
    {
        static bool from_data(const N &node, std::unordered_map<std::string, T> &value)
        {
            using map_type = N::map_type;

            if (!node.template is<map_type>())
                return false;

            auto ok = true;
            for (auto &&[key, val] : node)
                ok &= val >> value[key];

            return ok;
        }

        template<toolkit::same_as<std::unordered_map<std::string, T>> U>
        static void to_data(N &node, U &&value)
        {
            using map_type = N::map_type;

            node = map_type();

            for (auto &&[key, val] : value)
                node[key] = val;
        }
    };

    template<node_type N, typename T>
    struct serializer<N, std::optional<T>>
    {
        static bool from_data(const N &node, std::optional<T> &value)
        {
            if (!node)
            {
                value = std::nullopt;
                return true;
            }

            if (T val; node >> val)
            {
                value = std::move(val);
                return true;
            }

            return false;
        }

        template<toolkit::same_as<std::optional<T>> U>
        static void to_data(N &node, U &&value)
        {
            if (value.has_value())
            {
                node = value.value();
                return;
            }

            node = undefined_type();
        }
    };

    template<node_type N, typename... T>
    struct serializer<N, std::variant<T...>>
    {
        static bool from_data(const N &node, std::variant<T...> &value)
        {
            auto try_from_json = [&]<typename U>() -> bool
            {
                if (U val; node >> val)
                {
                    value = std::move(val);
                    return true;
                }
                return false;
            };

            return (try_from_json.template operator()<T>() || ...);
        }

        template<toolkit::same_as<std::variant<T...>> U>
        static void to_data(N &node, U &&value)
        {
            std::visit(
                [&node]<typename V>(V &&val)
                {
                    node = std::forward<V>(val);
                },
                std::forward<U>(value));
        }
    };
}

template<data::node_type N, typename T>
bool from_data_opt(const N &node, T &value, T default_value = {})
{
    if (std::optional<T> val; node >> val)
    {
        value = val.value_or(std::move(default_value));
        return true;
    }

    return false;
}

template<typename N, typename T>
bool data::from_data_fn(const N &node, T &value)
{
    return serializer<N, T>::from_data(node, value);
}

template<typename N, typename T>
void data::to_data_fn(N &node, T &&value)
{
    return serializer<N, std::decay_t<T>>::to_data(node, std::forward<T>(value));
}
