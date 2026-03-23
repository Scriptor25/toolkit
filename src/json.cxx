#include <cstdlib>
#include <iomanip>

#include <json.hxx>
#include <parser.hxx>
#include <utf8.hxx>

enum json_format
{
    compact,
    pretty,
};

struct json_context
{
    json_format format;
    unsigned depth;
};

static json_context &get_json_context(std::ostream &stream)
{
    static auto index = std::ios_base::xalloc();

    auto context = static_cast<json_context *>(stream.pword(index));
    if (!context)
    {
        context = new json_context({ .format = compact, .depth = 0 });
        stream.pword(index) = context;
    }

    return *context;
}

static std::ostream &depth_space(std::ostream &stream)
{
    const auto &[format, depth] = get_json_context(stream);

    for (unsigned i = 0; i < depth; ++i)
        stream << "  ";

    return stream;
}

json::NullNode::NullNode(Node &&node)
{
    if (!IsNull(node))
        throw std::runtime_error("error");

    node = {};
}

json::NullNode::NullNode(const Node &node)
{
    if (!IsNull(node))
        throw std::runtime_error("error");
}

std::ostream &json::NullNode::Print(std::ostream &stream) const
{
    return stream << "null";
}

json::BooleanNode::BooleanNode(const bool value)
    : Value(value)
{
}

json::BooleanNode::BooleanNode(Node &&node)
{
    if (!IsBoolean(node))
        throw std::runtime_error("error");

    Value = AsBoolean(node).Value;
    node = {};
}

json::BooleanNode::BooleanNode(const Node &node)
{
    if (!IsBoolean(node))
        throw std::runtime_error("error");

    Value = AsBoolean(node).Value;
}

json::BooleanNode::operator bool &()
{
    return Value;
}

json::BooleanNode::operator bool() const
{
    return Value;
}

std::ostream &json::BooleanNode::Print(std::ostream &stream) const
{
    return stream << (Value ? "true" : "false");
}

json::NumberNode::NumberNode(const long double value)
    : Value(value)
{
}

json::NumberNode::NumberNode(Node &&node)
{
    if (!IsNumber(node))
        throw std::runtime_error("error");

    Value = AsNumber(node).Value;
    node = {};
}

json::NumberNode::NumberNode(const Node &node)
{
    if (!IsNumber(node))
        throw std::runtime_error("error");

    Value = AsNumber(node).Value;
}

json::NumberNode::operator long double &()
{
    return Value;
}

json::NumberNode::operator long double() const
{
    return Value;
}

std::ostream &json::NumberNode::Print(std::ostream &stream) const
{
    return stream << std::scientific << Value;
}

json::StringNode::StringNode(const char *value)
    : Value(value)
{
}

json::StringNode::StringNode(std::string &&value)
    : Value(std::move(value))
{
}

json::StringNode::StringNode(const std::string &value)
    : Value(value)
{
}

json::StringNode::StringNode(Node &&node)
{
    if (!IsString(node))
        throw std::runtime_error("error");

    Value = std::move(AsString(node).Value);
    node = {};
}

json::StringNode::StringNode(const Node &node)
{
    if (!IsString(node))
        throw std::runtime_error("error");

    Value = AsString(node).Value;
}

json::StringNode::operator std::string &()
{
    return Value;
}

json::StringNode::operator const std::string &() const
{
    return Value;
}

std::ostream &json::StringNode::Print(std::ostream &stream) const
{
    stream << '"';

    for (auto i = Value.begin(); i != Value.end();)
        switch (const auto c = utf8::decode(i, Value.end()))
        {
        case '"':
            stream << "\\\"";
            break;
        case '\\':
            stream << "\\\\";
            break;
        case '\b':
            stream << "\\b";
            break;
        case '\f':
            stream << "\\f";
            break;
        case '\n':
            stream << "\\n";
            break;
        case '\r':
            stream << "\\r";
            break;
        case '\t':
            stream << "\\t";
            break;
        default:
            if (0x20 <= c && c < 0x7F)
                stream << static_cast<char>(c);
            else
                stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            break;
        }

    return stream << '"';
}

json::ArrayNode::ArrayNode(const std::size_t count)
    : Elements(count)
{
}

json::ArrayNode::ArrayNode(std::vector<Node> &&elements)
    : Elements(std::move(elements))
{
}

json::ArrayNode::ArrayNode(const std::vector<Node> &elements)
    : Elements(elements)
{
}

json::ArrayNode::ArrayNode(Node &&node)
{
    if (!IsArray(node))
        throw std::runtime_error("error");

    Elements = std::move(AsArray(node).Elements);
    node = {};
}

json::ArrayNode::ArrayNode(const Node &node)
{
    if (!IsArray(node))
        throw std::runtime_error("error");

    Elements = AsArray(node).Elements;
}

json::ArrayNode::operator std::vector<Node> &()
{
    return Elements;
}

json::ArrayNode::operator const std::vector<Node> &() const
{
    return Elements;
}

std::vector<json::Node>::iterator json::ArrayNode::begin()
{
    return Elements.begin();
}

std::vector<json::Node>::iterator json::ArrayNode::end()
{
    return Elements.end();
}

std::vector<json::Node>::const_iterator json::ArrayNode::begin() const
{
    return Elements.begin();
}

std::vector<json::Node>::const_iterator json::ArrayNode::end() const
{
    return Elements.end();
}

std::size_t json::ArrayNode::size() const
{
    return Elements.size();
}

json::Node &json::ArrayNode::operator[](const std::size_t index)
{
    return Elements[index];
}

const json::Node &json::ArrayNode::operator[](const std::size_t index) const
{
    return Elements[index];
}

std::ostream &json::ArrayNode::Print(std::ostream &stream) const
{
    auto &[format, depth] = get_json_context(stream);

    switch (format)
    {
    case json_format::pretty:
    {
        stream << '[';
        if (Elements.size() > 1)
            stream << '\n';
        depth++;
        for (auto it = Elements.begin(); it != Elements.end(); ++it)
        {
            if (it != Elements.begin())
                stream << ',' << '\n';
            if (Elements.size() > 1)
                stream << depth_space;
            stream << *it;
        }
        depth--;
        if (Elements.size() > 1)
            stream << '\n' << depth_space;
        return stream << ']';
    }

    case json_format::compact:
    default:
    {
        stream << '[';
        depth++;
        for (auto it = Elements.begin(); it != Elements.end(); ++it)
        {
            if (it != Elements.begin())
                stream << ',';
            stream << *it;
        }
        depth--;
        return stream << ']';
    }
    }
}

json::ObjectNode::ObjectNode(std::map<std::string, Node> &&elements)
    : Elements(std::move(elements))
{
}

json::ObjectNode::ObjectNode(const std::map<std::string, Node> &elements)
    : Elements(elements)
{
}

json::ObjectNode::ObjectNode(Node &&node)
{
    if (!IsObject(node))
        throw std::runtime_error("error");

    Elements = std::move(AsObject(node).Elements);
    node = {};
}

json::ObjectNode::ObjectNode(const Node &node)
{
    if (!IsObject(node))
        throw std::runtime_error("error");

    Elements = AsObject(node).Elements;
}

json::ObjectNode::operator std::map<std::string, Node> &()
{
    return Elements;
}

json::ObjectNode::operator const std::map<std::string, Node> &() const
{
    return Elements;
}

std::map<std::string, json::Node>::iterator json::ObjectNode::begin()
{
    return Elements.begin();
}

std::map<std::string, json::Node>::iterator json::ObjectNode::end()
{
    return Elements.end();
}

std::map<std::string, json::Node>::const_iterator json::ObjectNode::begin() const
{
    return Elements.begin();
}

std::map<std::string, json::Node>::const_iterator json::ObjectNode::end() const
{
    return Elements.end();
}

json::Node &json::ObjectNode::operator[](const std::string &key)
{
    return Elements[key];
}

json::Node json::ObjectNode::operator[](const std::string &key) const
{
    if (Elements.contains(key))
        return Elements.at(key);
    return {};
}

std::ostream &json::ObjectNode::Print(std::ostream &stream) const
{
    auto &[format, depth] = get_json_context(stream);

    switch (format)
    {
    case json_format::pretty:
    {
        stream << '{';
        if (!Elements.empty())
            stream << '\n';
        depth++;
        for (auto it = Elements.begin(); it != Elements.end(); ++it)
        {
            if (it != Elements.begin())
                stream << ',' << '\n';
            stream << depth_space << StringNode(it->first) << ": " << it->second;
        }
        depth--;
        if (!Elements.empty())
            stream << '\n' << depth_space;
        return stream << '}';
    }

    case json_format::compact:
    default:
    {
        stream << '{';
        depth++;
        for (auto it = Elements.begin(); it != Elements.end(); ++it)
        {
            if (it != Elements.begin())
                stream << ',';
            stream << StringNode(it->first) << ':' << it->second;
        }
        depth--;
        return stream << '}';
    }
    }
}

bool json::IsUndefined(const Node &node)
{
    return std::holds_alternative<std::monostate>(node);
}

bool json::IsNull(const Node &node)
{
    return std::holds_alternative<NullNode>(node);
}

bool json::IsBoolean(const Node &node)
{
    return std::holds_alternative<BooleanNode>(node);
}

bool json::IsNumber(const Node &node)
{
    return std::holds_alternative<NumberNode>(node);
}

bool json::IsString(const Node &node)
{
    return std::holds_alternative<StringNode>(node);
}

bool json::IsArray(const Node &node)
{
    return std::holds_alternative<ArrayNode>(node);
}

bool json::IsObject(const Node &node)
{
    return std::holds_alternative<ObjectNode>(node);
}

const json::NullNode &json::AsNull(const Node &node)
{
    return std::get<NullNode>(node);
}

const json::BooleanNode &json::AsBoolean(const Node &node)
{
    return std::get<BooleanNode>(node);
}

const json::NumberNode &json::AsNumber(const Node &node)
{
    return std::get<NumberNode>(node);
}

const json::StringNode &json::AsString(const Node &node)
{
    return std::get<StringNode>(node);
}

const json::ArrayNode &json::AsArray(const Node &node)
{
    return std::get<ArrayNode>(node);
}

const json::ObjectNode &json::AsObject(const Node &node)
{
    return std::get<ObjectNode>(node);
}

json::NullNode &json::AsNull(Node &node)
{
    return std::get<NullNode>(node);
}

json::BooleanNode &json::AsBoolean(Node &node)
{
    return std::get<BooleanNode>(node);
}

json::NumberNode &json::AsNumber(Node &node)
{
    return std::get<NumberNode>(node);
}

json::StringNode &json::AsString(Node &node)
{
    return std::get<StringNode>(node);
}

json::ArrayNode &json::AsArray(Node &node)
{
    return std::get<ArrayNode>(node);
}

json::ObjectNode &json::AsObject(Node &node)
{
    return std::get<ObjectNode>(node);
}

std::ostream &json::compact(std::ostream &stream)
{
    auto &[format, depth] = get_json_context(stream);
    format = json_format::compact;
    return stream;
}

std::ostream &json::pretty(std::ostream &stream)
{
    auto &[format, depth] = get_json_context(stream);
    format = json_format::pretty;
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const json::Node &node)
{
    if (json::IsUndefined(node))
        return stream << "<undefined>";

    if (json::IsNull(node))
        return json::AsNull(node).Print(stream);
    if (json::IsBoolean(node))
        return json::AsBoolean(node).Print(stream);
    if (json::IsNumber(node))
        return json::AsNumber(node).Print(stream);
    if (json::IsString(node))
        return json::AsString(node).Print(stream);
    if (json::IsArray(node))
        return json::AsArray(node).Print(stream);
    if (json::IsObject(node))
        return json::AsObject(node).Print(stream);

    return stream << "<error>";
}

std::istream &operator>>(std::istream &stream, json::Node &node)
{
    node = json::Parser(stream).Parse();
    return stream;
}

template<>
bool from_json(const json::Node &node, bool &value)
{
    if (json::IsBoolean(node))
    {
        value = json::AsBoolean(node);
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const bool &value)
{
    node = json::BooleanNode(value);
}

template<>
bool from_json(const json::Node &node, long double &value)
{
    if (json::IsNumber(node))
    {
        value = json::AsNumber(node);
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const long double &value)
{
    node = json::NumberNode(value);
}

template<>
bool from_json(const json::Node &node, std::string &value)
{
    if (json::IsString(node))
    {
        value = json::AsString(node);
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const std::string &value)
{
    node = json::StringNode(value);
}
