#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace json
{
    struct NullNode;
    struct BooleanNode;
    struct NumberNode;
    struct StringNode;
    struct ArrayNode;
    struct ObjectNode;

    using Node = std::variant<
        std::monostate,
        NullNode,
        BooleanNode,
        NumberNode,
        StringNode,
        ArrayNode,
        ObjectNode
    >;

    struct NullNode
    {
        NullNode() = default;

        NullNode(Node &&node);
        NullNode(const Node &node);

        std::ostream &Print(std::ostream &stream) const;
    };

    struct BooleanNode
    {
        BooleanNode() = default;

        BooleanNode(bool value);

        BooleanNode(Node &&node);
        BooleanNode(const Node &node);

        operator bool &();
        operator bool() const;

        std::ostream &Print(std::ostream &stream) const;

        bool Value{};
    };

    struct NumberNode
    {
        NumberNode() = default;

        NumberNode(long double value);

        NumberNode(Node &&node);
        NumberNode(const Node &node);

        operator long double &();
        operator long double() const;

        std::ostream &Print(std::ostream &stream) const;

        long double Value{};
    };

    struct StringNode
    {
        StringNode() = default;

        StringNode(const char *value);

        StringNode(std::string &&value);
        StringNode(const std::string &value);

        StringNode(Node &&node);
        StringNode(const Node &node);

        operator std::string &();
        operator const std::string &() const;

        std::ostream &Print(std::ostream &stream) const;

        std::string Value{};
    };

    struct ArrayNode
    {
        ArrayNode() = default;

        ArrayNode(std::size_t count);

        ArrayNode(std::vector<Node> &&elements);
        ArrayNode(const std::vector<Node> &elements);

        ArrayNode(Node &&node);
        ArrayNode(const Node &node);

        operator std::vector<Node> &();
        operator const std::vector<Node> &() const;

        std::vector<Node>::iterator begin();
        std::vector<Node>::iterator end();

        [[nodiscard]] std::vector<Node>::const_iterator begin() const;
        [[nodiscard]] std::vector<Node>::const_iterator end() const;

        [[nodiscard]] std::size_t size() const;

        Node &operator[](std::size_t index);
        const Node &operator[](std::size_t index) const;

        std::ostream &Print(std::ostream &stream) const;

        std::vector<Node> Elements;
    };

    struct ObjectNode
    {
        ObjectNode() = default;

        ObjectNode(std::map<std::string, Node> &&elements);
        ObjectNode(const std::map<std::string, Node> &elements);

        ObjectNode(Node &&node);
        ObjectNode(const Node &node);

        operator std::map<std::string, Node> &();
        operator const std::map<std::string, Node> &() const;

        std::map<std::string, Node>::iterator begin();
        std::map<std::string, Node>::iterator end();

        [[nodiscard]] std::map<std::string, Node>::const_iterator begin() const;
        [[nodiscard]] std::map<std::string, Node>::const_iterator end() const;

        Node &operator[](const std::string &key);
        Node operator[](const std::string &key) const;

        std::ostream &Print(std::ostream &stream) const;

        std::map<std::string, Node> Elements;
    };

    bool IsUndefined(const Node &node);
    bool IsNull(const Node &node);
    bool IsBoolean(const Node &node);
    bool IsNumber(const Node &node);
    bool IsString(const Node &node);
    bool IsArray(const Node &node);
    bool IsObject(const Node &node);

    const NullNode &AsNull(const Node &node);
    const BooleanNode &AsBoolean(const Node &node);
    const NumberNode &AsNumber(const Node &node);
    const StringNode &AsString(const Node &node);
    const ArrayNode &AsArray(const Node &node);
    const ObjectNode &AsObject(const Node &node);

    NullNode &AsNull(Node &node);
    BooleanNode &AsBoolean(Node &node);
    NumberNode &AsNumber(Node &node);
    StringNode &AsString(Node &node);
    ArrayNode &AsArray(Node &node);
    ObjectNode &AsObject(Node &node);

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
    if (!json::IsArray(node))
        return false;

    auto &array_node = json::AsArray(node);
    value.resize(array_node.size());

    auto ok = true;
    for (std::size_t i = 0; i < array_node.size(); ++i)
        ok &= from_json(array_node[i], value[i]);

    return ok;
}

template<typename T>
void to_json(json::Node &node, const std::vector<T> &value)
{
    json::ArrayNode array_node(value.size());

    for (std::size_t i = 0; i < value.size(); ++i)
        to_json(array_node[i], value[i]);

    node = std::move(array_node);
}

template<typename T, std::size_t N>
bool from_json(const json::Node &node, std::array<T, N> &value)
{
    if (!json::IsArray(node))
        return false;

    auto &array_node = json::AsArray(node);
    if (array_node.size() != N)
        return false;

    value.resize(N);

    auto ok = true;
    for (std::size_t i = 0; i < N; ++i)
        ok &= from_json(array_node[i], value[i]);

    return ok;
}

template<typename T, std::size_t N>
void to_json(json::Node &node, const std::array<T, N> &value)
{
    json::ArrayNode array_node(N);

    for (std::size_t i = 0; i < N; ++i)
        to_json(array_node[i], value[i]);

    node = std::move(array_node);
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
    json::ArrayNode array_node(value.size());

    for (auto s = value.begin(), d = array_node.begin(); s != value.end() && d != array_node.end(); ++s, ++d)
        to_json(*d, *s);

    node = std::move(array_node);
}

template<typename T>
bool from_json(const json::Node &node, std::map<std::string, T> &value)
{
    if (!json::IsObject(node))
        return false;

    auto &object_node = json::AsObject(node);

    auto ok = true;
    for (auto &[key, val] : object_node)
        ok &= from_json(val, value[key]);

    return ok;
}

template<typename T>
void to_json(json::Node &node, const std::map<std::string, T> &value)
{
    json::ObjectNode object_node;

    for (auto &[key, val] : value)
        to_json(object_node[key], val);

    node = std::move(object_node);
}

template<typename T>
bool from_json(const json::Node &node, std::optional<T> &value)
{
    if (json::IsUndefined(node) || json::IsNull(node))
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
