#pragma once

#include <json/concepts.hxx>
#include <json/forward.hxx>

template<json::node N, typename T>
bool from_json(N &&node, T &value);

template<typename T>
void to_json(json::Node &node, T &&value);

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
            using ::to_json;

            to_json(*this, std::forward<T>(value));
        }

        template<assignable T>
        auto &&operator=(T &&value)
        {
            using ::to_json;

            to_json(*this, std::forward<T>(value));
            return *this;
        }

        template<typename T>
        bool operator>>(T &value)
        {
            using ::from_json;

            return from_json(*this, value);
        }

        template<typename T>
        bool operator>>(T &value) const
        {
            using ::from_json;

            return from_json(*this, value);
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

    std::ostream &operator<<(std::ostream &stream, const Node &node);
    std::istream &operator>>(std::istream &stream, Node &node);
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
    node = Node(std::forward<T>(value));
}

template<json::node N, json::floating_point T>
bool from_json(N &&node, T &value)
{
    using ::from_json;

    if (json::Number val; from_json(std::forward<N>(node), val))
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::floating_point T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    to_json(node, static_cast<json::Number>(std::forward<T>(value)));
}

template<json::node N, json::integral T>
bool from_json(N &&node, T &value)
{
    using ::from_json;

    if (json::Number val; from_json(std::forward<N>(node), val))
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::integral T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    to_json(node, static_cast<json::Number>(std::forward<T>(value)));
}

template<json::node N, typename T>
bool from_json(N &&node, std::vector<T> &value)
{
    using ::from_json;

    if (!node.template Is<json::Array>())
        return false;

    value.resize(node.size());

    auto ok = true;
    for (std::size_t i = 0; i < node.size(); ++i)
        ok &= from_json(node[i], value[i]);

    return ok;
}

template<json::vector T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    node = Array(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        to_json(node[i], value[i]);
}

template<json::node N, typename T, std::size_t S>
bool from_json(N &&node, std::array<T, S> &value)
{
    using ::from_json;

    if (!node.template Is<json::Array>() || node.size() != S)
        return false;

    value.resize(S);

    auto ok = true;
    for (std::size_t i = 0; i < S; ++i)
        ok &= from_json(node[i], value[i]);

    return ok;
}

template<json::array T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    node = Array(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        to_json(node[i], value[i]);
}

template<json::node N, typename T>
bool from_json(N &&node, std::set<T> &value)
{
    using ::from_json;

    if (std::vector<T> val; from_json(std::forward<N>(node), val))
    {
        value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
        return true;
    }

    return false;
}

template<json::set T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    to_json(node, std::vector(value.begin(), value.end()));
}

template<json::node N, typename T>
bool from_json(N &&node, std::map<json::Key, T> &value)
{
    using ::from_json;

    if (!node.template Is<json::Object>())
        return false;

    auto ok = true;
    for (auto &&[key, val] : node)
        ok &= from_json(val, value[key]);

    return ok;
}

template<json::map T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    node = json::Object();

    for (auto &&[key, val] : value)
        to_json(node[key], val);
}

template<json::node N, typename T>
bool from_json(N &&node, std::optional<T> &value)
{
    using ::from_json;

    if (node.template Is<json::Undefined>() || node.template Is<json::Null>())
    {
        value = std::nullopt;
        return true;
    }

    if (T val; from_json(std::forward<N>(node), val))
    {
        value = std::move(val);
        return true;
    }

    return false;
}

template<json::optional T>
void to_json(json::Node &node, T &&value)
{
    using ::to_json;

    if (value.has_value())
    {
        to_json(node, value.value());
        return;
    }

    node = json::Undefined();
}

template<json::node N, typename... T>
bool from_json(N &&node, std::variant<T...> &value)
{
    using ::from_json;

    return ([&]() -> bool
    {
        if (T val; from_json(node, val))
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
    using ::to_json;

    std::visit(
        [&node]<typename V>(V &&val)
        {
            to_json(node, val);
        },
        std::forward<T>(value));
}

template<json::node N, typename T>
bool from_json_opt(N &&node, T &value, T default_value)
{
    using ::from_json;

    if (std::optional<T> val; from_json(std::forward<N>(node), val))
    {
        value = val.value_or(std::move(default_value));
        return true;
    }

    return false;
}
