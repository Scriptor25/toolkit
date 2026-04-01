#pragma once

#include <json/concepts.hxx>
#include <json/forward.hxx>

namespace json
{
    struct Node final
    {
        template<typename T, typename V, typename M>
        struct iterator_base final
        {
            using value_type = std::pair<Key, T &>;

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
                    auto operator()(const V &i) -> value_type
                    {
                        return { {}, *i };
                    }

                    auto operator()(const M &i) -> value_type
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

            std::variant<V, M> it{};
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
        Node &operator=(T &&value)
        {
            Value = std::forward<T>(value);
            return *this;
        }

        template<assignable T>
        Node(T &&value);

        template<assignable T>
        Node &operator=(T &&value);

        template<typename T>
        bool operator>>(T &value) const;

        template<primitive T>
        [[nodiscard]] auto Is() const
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

template<typename T>
bool from_json(const json::Node &node, T &value) = delete;

template<typename T>
void to_json(json::Node &node, T &&value) = delete;

template<json::primitive T>
bool from_json(const json::Node &node, T &value)
{
    if (node.Is<T>())
    {
        value = node.Get<T>();
        return true;
    }

    return false;
}

template<json::primitive T>
void to_json(json::Node &node, T &&value)
{
    node = json::Node(std::forward<T>(value));
}

template<json::floating_point T>
bool from_json(const json::Node &node, T &value)
{
    if (json::Number val; node >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::floating_point T>
void to_json(json::Node &node, T &&value)
{
    node = static_cast<json::Number>(std::forward<T>(value));
}

template<json::integral T>
bool from_json(const json::Node &node, T &value)
{
    if (json::Number val; node >> val)
    {
        value = static_cast<T>(val);
        return true;
    }

    return false;
}

template<json::integral T>
void to_json(json::Node &node, T &&value)
{
    node = static_cast<json::Number>(std::forward<T>(value));
}

template<typename T>
bool from_json(const json::Node &node, std::vector<T> &value)
{
    if (!node.Is<json::Array>())
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
    node = json::Array(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        node[i] = value[i];
}

template<typename T, std::size_t S>
bool from_json(const json::Node &node, std::array<T, S> &value)
{
    if (!node.Is<json::Array>() || node.size() != S)
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
    node = json::Array(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        node[i] = value[i];
}

template<typename T>
bool from_json(const json::Node &node, std::set<T> &value)
{
    if (std::vector<T> val; node >> val)
    {
        value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
        return true;
    }

    return false;
}

template<json::set T>
void to_json(json::Node &node, T &&value)
{
    node = std::vector(value.begin(), value.end());
}

template<typename T>
bool from_json(const json::Node &node, std::map<json::Key, T> &value)
{
    if (!node.Is<json::Object>())
        return false;

    auto ok = true;
    for (auto &&[key, val] : node)
        ok &= val >> value[key];

    return ok;
}

template<json::map T>
void to_json(json::Node &node, T &&value)
{
    node = json::Object();

    for (auto &&[key, val] : value)
        node[key] = val;
}

template<typename T>
bool from_json(const json::Node &node, std::optional<T> &value)
{
    if (node.Is<json::Undefined>() || node.Is<json::Null>())
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

template<json::optional T>
void to_json(json::Node &node, T &&value)
{
    if (value.has_value())
    {
        node = value.value();
        return;
    }

    node = json::Undefined();
}

template<typename... T>
bool from_json(const json::Node &node, std::variant<T...> &value)
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
            node = val;
        },
        std::forward<T>(value));
}

template<typename T>
bool from_json_opt(const json::Node &node, T &value, T default_value)
{
    if (std::optional<T> val; node >> val)
    {
        value = val.value_or(std::move(default_value));
        return true;
    }

    return false;
}

namespace json
{
    struct from_json_fn
    {
        template<typename T>
        bool operator()(const Node &node, T &value) const
        {
            using U = std::decay_t<T>;

            if constexpr (enable_from_json<U>)
            {
                return serializer<U>::from_json(node, value);
            }
            else
            {
                using ::from_json;
                return from_json(node, value);
            }
        }
    };

    inline constexpr from_json_fn from_json_dispatch;

    struct to_json_fn
    {
        template<typename T>
        void operator()(Node &node, T &&value) const
        {
            using U = std::decay_t<T>;

            if constexpr (enable_to_json<U>)
            {
                return serializer<U>::to_json(node, std::forward<T>(value));
            }
            else
            {
                using ::to_json;
                return to_json(node, std::forward<T>(value));
            }
        }
    };

    inline constexpr to_json_fn to_json_dispatch;

    template<assignable T>
    Node::Node(T &&value)
    {
        to_json_dispatch(*this, std::forward<T>(value));
    }

    template<assignable T>
    Node &Node::operator=(T &&value)
    {
        to_json_dispatch(*this, std::forward<T>(value));
        return *this;
    }

    template<typename T>
    bool Node::operator>>(T &value) const
    {
        return from_json_dispatch(*this, value);
    }
}
