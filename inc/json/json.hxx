#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace json
{
    struct Node;

    std::ostream &operator<<(std::ostream &stream, const Node &node);
    std::istream &operator>>(std::istream &stream, Node &node);

    using Index = std::size_t;
    using Key = std::string;

    using Undefined = std::monostate;
    using Null = std::nullptr_t;
    using Boolean = bool;
    using Number = long double;
    using String = std::string;
    using Array = std::vector<Node>;
    using Object = std::map<Key, Node>;

    using NodeValue = std::variant<
        Undefined,
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    >;

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
}

template<json::node N, typename T>
bool from_json(N &&node, T &value) = delete;

template<typename T>
void to_json(json::Node &node, T &&value) = delete;

template<json::node N>
bool from_json(N &&node, json::Node &value);

template<json::node T>
void to_json(json::Node &node, T &&value);

template<json::node N, json::primitive T>
bool from_json(N &&node, T &value);

template<json::primitive T>
void to_json(json::Node &node, T &&value);

template<json::node N, json::floating_point T>
bool from_json(N &&node, T &value);

template<json::floating_point T>
void to_json(json::Node &node, T &&value);

template<json::node N, json::integral T>
bool from_json(N &&node, T &value);

template<json::integral T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T>
bool from_json(N &&node, std::vector<T> &value);

template<json::vector T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T, std::size_t S>
bool from_json(N &&node, std::array<T, S> &value);

template<json::array T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T>
bool from_json(N &&node, std::set<T> &value);

template<json::set T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T>
bool from_json(N &&node, std::map<json::Key, T> &value);

template<json::map T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T>
bool from_json(N &&node, std::optional<T> &value);

template<json::optional T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename... T>
bool from_json(N &&node, std::variant<T...> &value);

template<json::variant T>
void to_json(json::Node &node, T &&value);

template<json::node N, typename T>
bool from_json_opt(N &&node, T &value, T default_value = {});

template<json::node N, typename T>
bool operator>>(N &&node, T &value)
{
    using ::from_json;
    return from_json(std::forward<N>(node), value);
}

template<typename T>
json::Node &operator<<(json::Node &node, T &&value)
{
    using ::to_json;
    to_json(node, std::forward<T>(value));
    return node;
}

namespace json
{
    struct Node final
    {
        template<typename T, typename V, typename M>
        struct iterator_base final
        {
            using value_type = std::pair<Key, T &>;

            iterator_base() = default;

            explicit iterator_base(V &&it)
                : it(std::forward<V>(it))
            {
            }

            explicit iterator_base(M &&it)
                : it(std::forward<M>(it))
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
                    auto operator()(V &&i) -> value_type
                    {
                        return { {}, *i };
                    }

                    auto operator()(M &&i) -> value_type
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

            std::variant<std::monostate, V, M> it{};
        };

        using iterator = iterator_base<
            Node,
            Array::iterator,
            Object::iterator>;
        using const_iterator = iterator_base<
            const Node,
            Array::const_iterator,
            Object::const_iterator>;

        Node() = default;

        Node(const Node &node) = default;
        Node &operator=(const Node &node) = default;

        Node(Node &&node) noexcept = default;
        Node &operator=(Node &&node) noexcept = default;

        explicit Node(const NodeValue &value);
        Node &operator=(const NodeValue &value);

        explicit Node(NodeValue &&value);
        Node &operator=(NodeValue &&value);

        template<primitive T>
        Node(T &&value)
            : Value(std::forward<T>(value))
        {
        }

        template<primitive T>
        auto &&operator=(T &&value)
        {
            Value = std::forward<T>(value);
            return *this;
        }

        template<assignable T>
        Node(T &&value)
        {
            *this << value;
        }

        template<assignable T>
        auto &&operator=(T &&value)
        {
            *this << value;
            return *this;
        }

        template<primitive T>
        auto Is() const
        {
            return std::holds_alternative<T>(Value);
        }

        template<primitive T>
        auto &&Get()
        {
            return std::get<T>(Value);
        }

        template<primitive T>
        auto &&Get() const
        {
            return std::get<T>(Value);
        }

        bool operator!() const;

        std::ostream &Print(std::ostream &stream) const;

        iterator begin();
        iterator end();

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;

        [[nodiscard]] Index size() const;

        Node &operator[](Index index);
        Node operator[](Index index) const;

        Node &operator[](const Key &key);
        Node operator[](const Key &key) const;

        NodeValue Value{};
    };
}

template<json::node N>
bool from_json(N &&node, json::Node &value)
{
    value = std::forward<N>(node);
    return true;
}

template<json::node T>
void to_json(json::Node &node, T &&value)
{
    node = std::forward<T>(value);
}

template<json::node N, json::primitive T>
bool from_json(N &&node, T &value)
{
    if (node.template Is<T>())
    {
        value = node.template Get<T>();
        return true;
    }

    return false;
}

template<json::primitive T>
void to_json(json::Node &node, T &&value)
{
    node = json::Node(std::forward<T>(value));
}

template<json::node N, json::floating_point T>
bool from_json(N &&node, T &value)
{
    if (json::Number val; std::forward<N>(node) >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::floating_point T>
void to_json(json::Node &node, T &&value)
{
    node << static_cast<json::Number>(std::forward<T>(value));
}

template<json::node N, json::integral T>
bool from_json(N &&node, T &value)
{
    if (json::Number val; std::forward<N>(node) >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::integral T>
void to_json(json::Node &node, T &&value)
{
    node << static_cast<json::Number>(std::forward<T>(value));
}

template<json::node N, typename T>
bool from_json(N &&node, std::vector<T> &value)
{
    if (!node.template Is<json::Array>())
        return false;

    value.resize(node.size());

    auto ok = true;
    for (std::size_t i = 0; i < node.size(); ++i)
        ok &= node[i] >> value[i];

    return ok;
}

template<json::vector T>
void to_json(json::Node &node, T &&value)
{
    json::Array nodes(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        nodes[i] << value[i];

    node = json::Node(std::move(nodes));
}

template<json::node N, typename T, std::size_t S>
bool from_json(N &&node, std::array<T, S> &value)
{
    if (!node.template Is<json::Array>() || node.size() != S)
        return false;

    value.resize(S);

    auto ok = true;
    for (std::size_t i = 0; i < S; ++i)
        ok &= node[i] >> value[i];

    return ok;
}

template<json::array T>
void to_json(json::Node &node, T &&value)
{
    json::Array nodes(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        nodes[i] << value[i];

    node = json::Node(std::move(nodes));
}

template<json::node N, typename T>
bool from_json(N &&node, std::set<T> &value)
{
    if (std::vector<T> val; std::forward<N>(node) >> val)
    {
        value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
        return true;
    }

    return false;
}

template<json::set T>
void to_json(json::Node &node, T &&value)
{
    node << std::vector(value.begin(), value.end());
}

template<json::node N, typename T>
bool from_json(N &&node, std::map<json::Key, T> &value)
{
    if (!node.template Is<json::Object>())
        return false;

    auto ok = true;
    for (auto &&[key, val] : std::forward<N>(node))
        ok &= std::forward<typeof(val)>(val) >> value[std::forward<typeof(key)>(key)];

    return ok;
}

template<json::map T>
void to_json(json::Node &node, T &&value)
{
    json::Object nodes;

    for (auto &&[key, val] : std::forward<T>(value))
        nodes[std::forward<typeof(key)>(key)] << std::forward<typeof(val)>(val);

    node = json::Node(std::move(nodes));
}

template<json::node N, typename T>
bool from_json(N &&node, std::optional<T> &value)
{
    if (node.template Is<json::Undefined>() || node.template Is<json::Null>())
    {
        value = std::nullopt;
        return true;
    }

    if (T element; std::forward<N>(node) >> element)
    {
        value = std::move(element);
        return true;
    }

    return false;
}

template<json::optional T>
void to_json(json::Node &node, T &&value)
{
    if (value.has_value())
    {
        node << value.value();
        return;
    }

    node = json::Node();
}

template<json::node N, typename... T>
bool from_json(N &&node, std::variant<T...> &value)
{
    return ([&]() -> bool
    {
        if (T val; node >> val)
        {
            value = std::move(val);
            return true;
        }
        return false;
    }() || ...);
}

template<json::variant T>
void to_json(json::Node &node, T &&value)
{
    std::visit(
        [&node]<typename V>(V &&val)
        {
            node << val;
        },
        std::forward<T>(value));
}

template<json::node N, typename T>
bool from_json_opt(N &&node, T &value, T default_value)
{
    if (std::optional<T> val; std::forward<N>(node) >> val)
    {
        value = val.value_or(std::move(default_value));
        return true;
    }

    return false;
}
