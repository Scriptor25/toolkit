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

    using NodeValue = std::variant<
        std::monostate,
        std::nullptr_t,
        bool,
        long double,
        std::string,
        std::vector<Node>,
        std::map<std::string, Node>
    >;

    struct Node final
    {
        template<typename T, typename V, typename M>
        struct iterator_base final
        {
            using entry_t = std::pair<std::string, T &>;

            iterator_base(V it, std::size_t idx)
                : it(it),
                  idx(idx)
            {
            }

            iterator_base(M it)
                : it(it)
            {
            }

            bool operator!=(iterator_base other) const
            {
                return it != other.it;
            }

            entry_t operator*() const
            {
                return std::visit(
                    [&]<typename I>(I &i) -> entry_t
                    {
                        if constexpr (std::is_same_v<I, V>)
                            return { std::to_string(idx), *i };
                        else
                            return { i->first, i->second };
                    },
                    it);
            }

            iterator_base &operator++()
            {
                std::visit(
                    [](auto &it)
                    {
                        ++it;
                    },
                    it);

                if (std::holds_alternative<V>(it))
                    ++idx;

                return *this;
            }

            std::variant<V, M> it;
            std::size_t idx{};
        };

        using iterator = iterator_base<
            Node,
            std::vector<Node>::iterator,
            std::map<std::string, Node>::iterator>;
        using const_iterator = iterator_base<
            const Node,
            std::vector<Node>::const_iterator,
            std::map<std::string, Node>::const_iterator>;

        Node() = default;

        Node(NodeValue &&value);
        Node(const NodeValue &value);

        [[nodiscard]] bool IsUndefined() const;
        [[nodiscard]] bool IsNull() const;
        [[nodiscard]] bool IsBoolean() const;
        [[nodiscard]] bool IsNumber() const;
        [[nodiscard]] bool IsString() const;
        [[nodiscard]] bool IsArray() const;
        [[nodiscard]] bool IsObject() const;

        bool &GetBoolean();
        [[nodiscard]] const bool &GetBoolean() const;

        long double &GetNumber();
        [[nodiscard]] const long double &GetNumber() const;

        std::string &GetString();
        [[nodiscard]] const std::string &GetString() const;

        std::vector<Node> &GetArray();
        [[nodiscard]] const std::vector<Node> &GetArray() const;

        std::map<std::string, Node> &GetObject();
        [[nodiscard]] const std::map<std::string, Node> &GetObject() const;

        std::ostream &Print(std::ostream &stream) const;

        iterator begin();
        iterator end();

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;

        [[nodiscard]] std::size_t size() const;

        Node &operator[](std::size_t index);
        Node operator[](std::size_t index) const;

        Node &operator[](const std::string &key);
        Node operator[](const std::string &key) const;

        NodeValue Value{};
    };

    std::ostream &compact(std::ostream &stream);
    std::ostream &pretty(std::ostream &stream);
}

std::ostream &operator<<(std::ostream &stream, const json::Node &node);
std::istream &operator>>(std::istream &stream, json::Node &node);

template<typename T>
bool from_json(const json::Node &node, T &value) = delete;

template<typename T>
void to_json(json::Node &node, const T &value) = delete;

template<>
bool from_json(const json::Node &node, bool &value);

template<>
void to_json(json::Node &node, const bool &value);

template<>
bool from_json(const json::Node &node, long double &value);

template<>
void to_json(json::Node &node, const long double &value);

template<>
bool from_json(const json::Node &node, std::string &value);

template<>
void to_json(json::Node &node, const std::string &value);

template<typename T>
bool from_json(const json::Node &node, std::vector<T> &value);

template<typename T>
void to_json(json::Node &node, const std::vector<T> &value);

template<typename T, std::size_t N>
bool from_json(const json::Node &node, std::array<T, N> &value);

template<typename T, std::size_t N>
void to_json(json::Node &node, const std::array<T, N> &value);

template<typename T>
bool from_json(const json::Node &node, std::set<T> &value);

template<typename T>
void to_json(json::Node &node, const std::set<T> &value);

template<typename T>
bool from_json(const json::Node &node, std::map<std::string, T> &value);

template<typename T>
void to_json(json::Node &node, const std::map<std::string, T> &value);

template<typename T>
bool from_json(const json::Node &node, std::optional<T> &value);

template<typename T>
void to_json(json::Node &node, const std::optional<T> &value);

template<typename T>
bool from_json(const json::Node &node, std::vector<T> &value)
{
    if (!node.IsArray())
        return false;

    value.resize(node.size());

    auto ok = true;
    for (std::size_t i = 0; i < node.size(); ++i)
        ok &= from_json(node[i], value[i]);

    return ok;
}

template<typename T>
void to_json(json::Node &node, const std::vector<T> &value)
{
    std::vector<json::Node> nodes(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        to_json(nodes[i], value[i]);

    node = { std::move(nodes) };
}

template<typename T, std::size_t N>
bool from_json(const json::Node &node, std::array<T, N> &value)
{
    if (!node.IsArray() || node.size() != N)
        return false;

    value.resize(N);

    auto ok = true;
    for (std::size_t i = 0; i < N; ++i)
        ok &= from_json(node[i], value[i]);

    return ok;
}

template<typename T, std::size_t N>
void to_json(json::Node &node, const std::array<T, N> &value)
{
    std::vector<json::Node> nodes(N);

    for (std::size_t i = 0; i < N; ++i)
        to_json(nodes[i], value[i]);

    node = { std::move(nodes) };
}

template<typename T>
bool from_json(const json::Node &node, std::set<T> &value)
{
    if (std::vector<T> vec; from_json(node, vec))
    {
        value = { std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()) };
        return true;
    }

    return false;
}

template<typename T>
void to_json(json::Node &node, const std::set<T> &value)
{
    to_json(node, std::vector(value.begin(), value.end()));
}

template<typename T>
bool from_json(const json::Node &node, std::map<std::string, T> &value)
{
    if (!node.IsObject())
        return false;

    auto ok = true;
    for (auto [key, val] : node)
        ok &= from_json(val, value[key]);

    return ok;
}

template<typename T>
void to_json(json::Node &node, const std::map<std::string, T> &value)
{
    std::map<std::string, json::Node> nodes;

    for (auto &[key, val] : value)
        to_json(nodes[key], val);

    node = { std::move(nodes) };
}

template<typename T>
bool from_json(const json::Node &node, std::optional<T> &value)
{
    if (node.IsUndefined() || node.IsNull())
    {
        value = std::nullopt;
        return true;
    }

    if (T element; from_json(node, element))
    {
        value = std::move(element);
        return true;
    }

    return false;
}

template<typename T>
void to_json(json::Node &node, const std::optional<T> &value)
{
    if (value.has_value())
        return to_json(node, value.value());

    node = {};
}

template<typename T>
bool from_json_opt(const json::Node &node, T &value, T default_value = {});

template<typename T>
bool from_json_opt(const json::Node &node, T &value, T default_value)
{
    if (std::optional<T> opt; from_json(node, opt))
    {
        value = std::move(opt.value_or(std::move(default_value)));
        return true;
    }

    return false;
}

template<typename T>
bool operator>>(const json::Node &node, T &value)
{
    return from_json(node, value);
}

template<typename T>
json::Node &operator<<(json::Node &node, const T &value)
{
    to_json(node, value);
    return node;
}
