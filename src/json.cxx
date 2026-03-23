#include <iomanip>

#include <json.hxx>
#include <parser.hxx>
#include <utf8.hxx>
#include <variant>

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

json::Node::Node(NodeValue &&value)
    : Value(std::forward<NodeValue>(value))
{
}

json::Node::Node(const NodeValue &value)
    : Value(value)
{
}

bool json::Node::IsUndefined() const
{
    return std::holds_alternative<std::monostate>(Value);
}

bool json::Node::IsNull() const
{
    return std::holds_alternative<std::nullptr_t>(Value);
}

bool json::Node::IsBoolean() const
{
    return std::holds_alternative<bool>(Value);
}

bool json::Node::IsNumber() const
{
    return std::holds_alternative<long double>(Value);
}

bool json::Node::IsString() const
{
    return std::holds_alternative<std::string>(Value);
}

bool json::Node::IsArray() const
{
    return std::holds_alternative<std::vector<Node>>(Value);
}

bool json::Node::IsObject() const
{
    return std::holds_alternative<std::map<std::string, Node>>(Value);
}

std::monostate &json::Node::GetUndefined()
{
    return std::get<std::monostate>(Value);
}

const std::monostate &json::Node::GetUndefined() const
{
    return std::get<std::monostate>(Value);
}

std::nullptr_t &json::Node::GetNull()
{
    return std::get<std::nullptr_t>(Value);
}

const std::nullptr_t &json::Node::GetNull() const
{
    return std::get<std::nullptr_t>(Value);
}

bool &json::Node::GetBoolean()
{
    return std::get<bool>(Value);
}

const bool &json::Node::GetBoolean() const
{
    return std::get<bool>(Value);
}

long double &json::Node::GetNumber()
{
    return std::get<long double>(Value);
}

const long double &json::Node::GetNumber() const
{
    return std::get<long double>(Value);
}

std::string &json::Node::GetString()
{
    return std::get<std::string>(Value);
}

const std::string &json::Node::GetString() const
{
    return std::get<std::string>(Value);
}

std::vector<json::Node> &json::Node::GetArray()
{
    return std::get<std::vector<Node>>(Value);
}

const std::vector<json::Node> &json::Node::GetArray() const
{
    return std::get<std::vector<Node>>(Value);
}

std::map<std::string, json::Node> &json::Node::GetObject()
{
    return std::get<std::map<std::string, Node>>(Value);
}

const std::map<std::string, json::Node> &json::Node::GetObject() const
{
    return std::get<std::map<std::string, Node>>(Value);
}

std::ostream &json::Node::Print(std::ostream &stream) const
{
    struct {
        void operator()(std::monostate) const
        {
            stream << "<undefined>";
        };

        void operator()(std::nullptr_t) const
        {
            stream << "null";
        };

        void operator()(const bool value) const
        {
            stream << (value ? "true" : "false");
        };

        void operator()(const long double value) const
        {
            stream << std::scientific << value;
        };

        void operator()(const std::string &value) const
        {
            stream << '"';

            for (auto i = value.begin(); i != value.end();)
                switch (const auto c = utf8::decode(i, value.end()))
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

            stream << '"';
        };

        void operator()(const std::vector<Node> &value) const
        {
            auto &[format, depth] = get_json_context(stream);

            switch (format)
            {
            case json_format::pretty:
            {
                stream << '[';
                if (value.size() > 1)
                    stream << '\n';
                depth++;
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',' << '\n';
                    if (value.size() > 1)
                        stream << depth_space;
                    stream << *it;
                }
                depth--;
                if (value.size() > 1)
                    stream << '\n' << depth_space;
                stream << ']';
                break;
            }

            case json_format::compact:
            default:
            {
                stream << '[';
                depth++;
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    stream << *it;
                }
                depth--;
                stream << ']';
                break;
            }
            }
        };

        void operator()(const std::map<std::string, Node> &value) const
        {
            auto &[format, depth] = get_json_context(stream);

            switch (format)
            {
            case json_format::pretty:
            {
                stream << '{';
                if (!value.empty())
                    stream << '\n';
                depth++;
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',' << '\n';
                    stream << depth_space << Node(it->first) << ": " << it->second;
                }
                depth--;
                if (!value.empty())
                    stream << '\n' << depth_space;
                stream << '}';
                break;
            }

            case json_format::compact:
            default:
            {
                stream << '{';
                depth++;
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    stream << Node(it->first) << ':' << it->second;
                }
                depth--;
                stream << '}';
                break;
            }
            }
        };

        std::ostream &stream;
    } visitor { stream };

    std::visit(visitor, Value);

    return stream;
}

json::Node::iterator json::Node::begin()
{
    if (IsArray())
        return { GetArray().begin(), 0 };
    return { GetObject().begin() };
}

json::Node::iterator json::Node::end()
{
    if (IsArray())
    {
        auto &array = GetArray();
        return { array.end(), array.size() };
    }
    return { GetObject().end() };
}

json::Node::const_iterator json::Node::begin() const
{
    if (IsArray())
        return { GetArray().begin(), 0 };
    return { GetObject().begin() };
}

json::Node::const_iterator json::Node::end() const
{
    if (IsArray())
    {
        auto &array = GetArray();
        return { array.end(), array.size() };
    }
    return { GetObject().end() };
}

std::size_t json::Node::size() const
{
    if (IsArray())
        return GetArray().size();
    return GetObject().size();
}

json::Node &json::Node::operator[](std::size_t index)
{
    return GetArray()[index];
}

const json::Node &json::Node::operator[](std::size_t index) const
{
    return GetArray()[index];
}

json::Node &json::Node::operator[](const std::string &key)
{
    return GetObject()[key];
}

json::Node json::Node::operator[](const std::string &key) const
{
    auto &map = GetObject();
    if (map.contains(key))
        return map.at(key);
    return {};
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
    return node.Print(stream);
}

std::istream &operator>>(std::istream &stream, json::Node &node)
{
    node = json::Parser(stream).Parse();
    return stream;
}

template<>
bool from_json(const json::Node &node, bool &value)
{
    if (node.IsBoolean())
    {
        value = node.GetBoolean();
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const bool &value)
{
    node = { value };
}

template<>
bool from_json(const json::Node &node, long double &value)
{
    if (node.IsNumber())
    {
        value = node.GetNumber();
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const long double &value)
{
    node = { value };
}

template<>
bool from_json(const json::Node &node, std::string &value)
{
    if (node.IsString())
    {
        value = node.GetString();
        return true;
    }

    return false;
}

template<>
void to_json(json::Node &node, const std::string &value)
{
    node = { value };
}
