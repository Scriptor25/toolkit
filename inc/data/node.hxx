#pragma once

#include <data/concepts.hxx>
#include <data/serializer.hxx>

#include <stdexcept>
#include <utility>
#include <variant>

namespace data
{
    template<typename N, typename T>
    bool from_data_fn(const N &node, T &value);

    template<typename N, typename T>
    void to_data_fn(N &node, T &&value);

    template<typename...>
    struct NodeTraits;

    template<typename... V>
    struct Node
    {
        using Traits = NodeTraits<V...>;

        using Index = unsigned long long;
        using Key = std::string;

        using Integer = Traits::Integer;
        using FloatingPoint = Traits::FloatingPoint;

        using Undefined = std::monostate;

        using Vec = std::vector<Node>;
        using Map = std::map<Key, Node>;

        using ValueType = std::variant<Undefined, V..., Vec, Map>;

        template<typename T, typename VI, typename MI>
        struct iterator_base final
        {
            using value_type = std::pair<Key, T &>;

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
            Node,
            typename Vec::iterator,
            typename Map::iterator>;
        using const_iterator = iterator_base<
            const Node,
            typename Vec::const_iterator,
            typename Map::const_iterator>;

        Node()
            : Value()
        {
        }

        Node(const Node &node) = default;
        Node &operator=(const Node &node) = default;

        Node(Node &&node) noexcept = default;
        Node &operator=(Node &&node) noexcept = default;

        explicit Node(const ValueType &value)
            : Value(value)
        {
        }

        Node &operator=(const ValueType &value)
        {
            Value = value;
            return *this;
        }

        explicit Node(ValueType &&value)
            : Value(std::forward<ValueType>(value))
        {
        }

        Node &operator=(ValueType &&value)
        {
            Value = value;
            return *this;
        }

        template<primitive<Node> T>
        Node(T &&value)
            : Value(std::forward<T>(value))
        {
        }

        template<primitive<Node> T>
        Node &operator=(T &&value)
        {
            Value = std::forward<T>(value);
            return *this;
        }

        template<assignable<Node, ValueType> T>
        Node(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
        }

        template<assignable<Node, ValueType> T>
        Node &operator=(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
            return *this;
        }

        template<typename T>
        bool operator>>(T &value) const
        {
            return from_data_fn(*this, value);
        }

        template<primitive<Node> T>
        [[nodiscard]] auto Is() const
        {
            return std::holds_alternative<T>(Value);
        }

        template<primitive<Node> T>
        auto &&Get()
        {
            return std::get<T>(Value);
        }

        template<primitive<Node> T>
        auto &&Get() const
        {
            return std::get<T>(Value);
        }

        bool operator!() const
        {
            return Is<Undefined>();
        }

        std::ostream &Print(std::ostream &stream, unsigned indent) const
        {
            return Traits::print_fn(stream, indent, Value);
        }

        iterator begin()
        {
            return std::visit(
                []<typename T>(T &value) -> iterator
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec> || std::same_as<U, Map>)
                        return iterator(value.begin());
                    else
                        throw std::runtime_error("type does not have `begin()`");
                },
                Value);
        }

        iterator end()
        {
            return std::visit(
                []<typename T>(T &value) -> iterator
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec> || std::same_as<U, Map>)
                        return iterator(value.end());
                    else
                        throw std::runtime_error("type does not have `end()`");
                },
                Value);
        }

        [[nodiscard]] const_iterator begin() const
        {
            return std::visit(
                []<typename T>(T &value) -> const_iterator
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec> || std::same_as<U, Map>)
                        return const_iterator(value.begin());
                    else
                        throw std::runtime_error("type does not have `begin() const`");
                },
                Value);
        }

        [[nodiscard]] const_iterator end() const
        {
            return std::visit(
                []<typename T>(T &value) -> const_iterator
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec> || std::same_as<U, Map>)
                        return const_iterator(value.end());
                    else
                        throw std::runtime_error("type does not have `end() const`");
                },
                Value);
        }

        [[nodiscard]] Index size() const
        {
            return std::visit(
                []<typename T>(T &value) -> Index
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec> || std::same_as<U, Map>)
                        return value.size();
                    else if constexpr (std::same_as<U, Undefined>)
                        return 0;
                    else
                        throw std::runtime_error("type does not have `size() const`");
                },
                Value);
        }

        Node &operator[](Index index)
        {
            static Node undefined;

            return std::visit(
                [&index]<typename T>(T &value) -> Node &
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec>)
                    {
                        if (index >= value.size())
                            value.resize(index + 1);
                        return value[index];
                    }
                    else if constexpr (std::same_as<U, Undefined>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Index)`");
                },
                Value);
        }

        const Node &operator[](Index index) const
        {
            static const Node undefined;

            return std::visit(
                [&index]<typename T>(T &value) -> const Node &
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Vec>)
                        return index < value.size() ? value[index] : undefined;
                    else if constexpr (std::same_as<U, Undefined>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Index) const`");
                },
                Value);
        }

        Node &operator[](const Key &key)
        {
            static Node undefined;

            return std::visit(
                [&key]<typename T>(T &value) -> Node &
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Map>)
                        return value[key];
                    else if constexpr (std::same_as<U, Undefined>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Key)`");
                },
                Value);
        }

        const Node &operator[](const Key &key) const
        {
            static const Node undefined;

            return std::visit(
                [&key]<typename T>(T &value) -> const Node &
                {
                    using U = std::decay_t<T>;

                    if constexpr (std::same_as<U, Map>)
                        return value.contains(key) ? value.at(key) : undefined;
                    else if constexpr (std::same_as<U, Undefined>)
                        return undefined;
                    else
                        throw std::runtime_error("type does not have `operator[](Key) const`");
                },
                Value);
        }

        ValueType Value;
    };
}

template<data::node N>
bool from_data(const N &node, N &value)
{
    value = node;
    return true;
}

template<data::node N, data::decay_same_as<N> T>
void to_data(N &node, T &&value)
{
    node = std::forward<T>(value);
}

template<data::node N, data::primitive<N> T>
bool from_data(const N &node, T &value)
{
    if (node.template Is<T>())
    {
        value = node.template Get<T>();
        return true;
    }

    return false;
}

template<data::node N, data::primitive<N> T>
void to_data(N &node, T &&value)
{
    node = N(std::forward<T>(value));
}

template<data::node N, data::floating_point<N> T>
bool from_data(const N &node, T &value)
{
    using F = N::FloatingPoint;

    if (F val; node >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<data::node N, data::floating_point<N> T>
void to_data(N &node, T &&value)
{
    using F = N::FloatingPoint;

    node = static_cast<F>(std::forward<T>(value));
}

template<data::node N, data::integral<N> T>
bool from_data(const N &node, T &value)
{
    using I = N::Integer;

    if (I val; node >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<data::node N, data::integral<N> T>
void to_data(N &node, T &&value)
{
    using I = N::Integer;

    node = static_cast<I>(std::forward<T>(value));
}

template<data::node N, typename T>
bool from_data(const N &node, std::vector<T> &value)
{
    using Vec = N::Vec;

    if (!node.template Is<Vec>())
        return false;

    value.resize(node.size());

    auto ok = true;
    for (std::size_t i = 0; i < node.size(); ++i)
        ok &= node[i] >> value[i];

    return ok;
}

template<data::node N, data::vector T>
void to_data(N &node, T &&value)
{
    using Vec = N::Vec;

    node = Vec(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        node[i] = value[i];
}

template<data::node N, typename T, std::size_t S>
bool from_data(const N &node, std::array<T, S> &value)
{
    using Vec = N::Vec;

    if (!node.template Is<Vec>() || node.size() != S)
        return false;

    value.resize(S);

    auto ok = true;
    for (std::size_t i = 0; i < S; ++i)
        ok &= node[i] >> value[i];

    return ok;
}

template<data::node N, data::array T>
void to_data(N &node, T &&value)
{
    using Vec = N::Vec;

    node = Vec(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        node[i] = value[i];
}

template<data::node N, typename T>
bool from_data(const N &node, std::set<T> &value)
{
    if (std::vector<T> val; node >> val)
    {
        value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
        return true;
    }

    return false;
}

template<data::node N, data::set T>
void to_data(N &node, T &&value)
{
    node = std::vector(value.begin(), value.end());
}

template<data::node N, typename T>
bool from_data(const N &node, std::map<typename N::Key, T> &value)
{
    using Map = N::Map;

    if (!node.template Is<Map>())
        return false;

    auto ok = true;
    for (auto &&[key, val] : node)
        ok &= val >> value[key];

    return ok;
}

template<data::node N, data::map T>
void to_data(N &node, T &&value)
{
    using Map = N::Map;

    node = Map();

    for (auto &&[key, val] : value)
        node[key] = val;
}

template<data::node N, typename T>
bool from_data(const N &node, std::optional<T> &value)
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

template<data::node N, data::optional T>
void to_data(N &node, T &&value)
{
    using Undefined = N::Undefined;

    if (value.has_value())
    {
        node = value.value();
        return;
    }

    node = Undefined();
}

template<data::node N, typename... T>
bool from_data(const N &node, std::variant<T...> &value)
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

template<data::node N, data::variant T>
void to_data(N &node, T &&value)
{
    std::visit(
        [&node]<typename V>(V &&val)
        {
            node = std::forward<V>(val);
        },
        std::forward<T>(value));
}

template<data::node N, typename T>
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
    using U = std::decay_t<T>;

    if constexpr (enable_from_data<N, U>)
    {
        return serializer<U>::from_data(node, value);
    }
    else
    {
        using ::from_data;
        return from_data(node, value);
    }
}

template<typename N, typename T>
void data::to_data_fn(N &node, T &&value)
{
    using U = std::decay_t<T>;

    if constexpr (enable_to_data<N, U>)
    {
        return serializer<U>::to_data(node, std::forward<T>(value));
    }
    else
    {
        using ::to_data;
        return to_data(node, std::forward<T>(value));
    }
}
