#include <json/json.hxx>
#include <json/parser.hxx>
#include <json/utf8.hxx>

#include <ostream>

static auto &get_context_depth(std::ostream &stream)
{
    static const auto index = std::ios_base::xalloc();

    return stream.iword(index);
}

static std::ostream &depth_space(std::ostream &stream)
{
    const auto &depth = get_context_depth(stream);

    const auto width = stream.width();
    const std::string buffer(width, ' ');

    for (unsigned i = 0; i < depth; ++i)
        stream << buffer;

    return stream;
}

json::Node::Node(const NodeValue &value)
    : Value(value)
{
}

json::Node::Node(NodeValue &&value)
    : Value(std::forward<NodeValue>(value))
{
}

json::Node &json::Node::operator=(const NodeValue &value)
{
    Value = value;
    return *this;
}

json::Node &json::Node::operator=(NodeValue &&value)
{
    Value = std::forward<NodeValue>(value);
    return *this;
}

bool json::Node::operator!() const
{
    return Is<Undefined>();
}

std::ostream &json::Node::Print(std::ostream &stream) const
{
    struct
    {
        void operator()(Undefined) const
        {
            stream << "<undefined>";
        }

        void operator()(Null) const
        {
            stream << "null";
        }

        void operator()(const Boolean value) const
        {
            stream << (value ? "true" : "false");
        }

        void operator()(const Integer value) const
        {
            stream << value;
        }

        void operator()(const FloatingPoint value) const
        {
            stream << std::scientific << value;
        }

        void operator()(const String &value) const
        {
            stream << '"';

            for (const auto c : utf8::decode(value))
                switch (c)
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
                    {
                        auto val = static_cast<int>(c);
                        stream << "\\u";

                        char buffer[5];
                        int x = sizeof(buffer) - 1;
                        buffer[x--] = '\0';

                        for (; x >= 0 && val > 0; --x)
                        {
                            auto [quot, rem] = std::div(val, 0x10);
                            buffer[x] = static_cast<char>(rem < 10 ? rem + '0' : rem - 10 + 'A');
                            val = quot;
                        }

                        for (; x >= 0; --x)
                            buffer[x] = '0';

                        stream << buffer;
                    }
                    break;
                }

            stream << '"';
        }

        void operator()(const Array &value) const
        {
            if (stream.width())
            {
                auto &depth = get_context_depth(stream);

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
            }
            else
            {
                stream << '[';
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    stream << *it;
                }
                stream << ']';
            }
        }

        void operator()(const Object &value) const
        {
            if (stream.width())
            {
                auto &depth = get_context_depth(stream);

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
            }
            else
            {
                stream << '{';
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    stream << Node(it->first) << ':' << it->second;
                }
                stream << '}';
            }
        }

        std::ostream &stream;
    } visitor{ stream };

    std::visit(visitor, Value);

    return stream;
}

json::Node::iterator json::Node::begin()
{
    return std::visit(
        []<typename T>(T &value) -> iterator
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array> || std::same_as<U, Object>)
                return iterator(value.begin());
            else
                throw std::runtime_error("type does not have `begin()`");
        },
        Value);
}

json::Node::iterator json::Node::end()
{
    return std::visit(
        []<typename T>(T &value) -> iterator
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array> || std::same_as<U, Object>)
                return iterator(value.end());
            else
                throw std::runtime_error("type does not have `end()`");
        },
        Value);
}

json::Node::const_iterator json::Node::begin() const
{
    return std::visit(
        []<typename T>(T &value) -> const_iterator
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array> || std::same_as<U, Object>)
                return const_iterator(value.begin());
            else
                throw std::runtime_error("type does not have `begin() const`");
        },
        Value);
}

json::Node::const_iterator json::Node::end() const
{
    return std::visit(
        []<typename T>(T &value) -> const_iterator
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array> || std::same_as<U, Object>)
                return const_iterator(value.end());
            else
                throw std::runtime_error("type does not have `end() const`");
        },
        Value);
}

json::Index json::Node::size() const
{
    return std::visit(
        []<typename T>(T &value) -> Index
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array> || std::same_as<U, Object>)
                return value.size();
            else if constexpr (std::same_as<U, Undefined>)
                return 0;
            else
                throw std::runtime_error("type does not have `size() const`");
        },
        Value);
}

static json::Node undefined;

json::Node &json::Node::operator[](const Index index)
{
    return std::visit(
        [&index]<typename T>(T &value) -> Node&
        {
            using U = std::decay_t<T>;

            if constexpr (std::same_as<U, Array>)
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

json::Node json::Node::operator[](const Index index) const
{
    return std::visit(
        [&index]<typename T>(T &value) -> const Node&
        {
            using U = std::decay_t<T>;
            
            if constexpr (std::same_as<U, Array>)
                return index < value.size() ? value[index] : undefined;
            else if constexpr (std::same_as<U, Undefined>)
                return undefined;
            else
                throw std::runtime_error("type does not have `operator[](Index) const`");
        },
        Value);
}

json::Node &json::Node::operator[](const std::string &key)
{
    return std::visit(
        [&key]<typename T>(T &value) -> Node&
        {
            using U = std::decay_t<T>;
            
            if constexpr (std::same_as<U, Object>)
                return value[key];
            else if constexpr (std::same_as<U, Undefined>)
                return undefined;
            else
                throw std::runtime_error("type does not have `operator[](Key)`");
        },
        Value);
}

json::Node json::Node::operator[](const std::string &key) const
{
    return std::visit(
        [&key]<typename T>(T &value) -> const Node&
        {
            using U = std::decay_t<T>;
            
            if constexpr (std::same_as<U, Object>)
                return value.contains(key) ? value.at(key) : undefined;
            else if constexpr (std::same_as<U, Undefined>)
                return undefined;
            else
                throw std::runtime_error("type does not have `operator[](Key) const`");
        },
        Value);
}

std::ostream &json::operator<<(std::ostream &stream, const Node &node)
{
    return node.Print(stream);
}

std::istream &json::operator>>(std::istream &stream, Node &node)
{
    node = Parser(stream).Parse();
    return stream;
}
