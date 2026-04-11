#include <data/utf8.hxx>
#include <json/json.hxx>
#include <json/parser.hxx>

#include <iomanip>
#include <iostream>
#include <ostream>

static auto &get_context_depth(std::ostream &stream)
{
    static const auto index = std::ios_base::xalloc();

    return stream.iword(index);
}

static std::ostream &indent_depth(std::ostream &stream, const std::size_t indent)
{
    const auto &depth = get_context_depth(stream);

    return stream << std::string(indent * depth, ' ');
}

std::ostream &data::NodeTraits<
    json::Null,
    json::Boolean,
    json::Integer,
    json::FloatingPoint,
    json::String
>::print_fn(std::ostream &stream, const unsigned indent, const json::Node::ValueType &value)
{
    struct
    {
        void operator()(json::Node::Undefined) const
        {
            stream << "<undefined>";
        }

        void operator()(json::Null) const
        {
            stream << "null";
        }

        void operator()(const json::Boolean value) const
        {
            stream << (value ? "true" : "false");
        }

        void operator()(const json::Integer value) const
        {
            stream << value;
        }

        void operator()(const json::FloatingPoint value) const
        {
            stream << std::scientific << value;
        }

        void operator()(const json::String &value) const
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
                        stream << "\\u" << std::setw(4) << std::setfill('0') << std::hex << static_cast<int>(c);
                    break;
                }

            stream << '"';
        }

        void operator()(const json::Node::Vec &value) const
        {
            if (indent)
            {
                auto &depth = get_context_depth(stream);

                stream << '[';
                if (value.size() > 1)
                {
                    stream << '\n';
                    depth++;
                }
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',' << '\n';
                    if (value.size() > 1)
                        indent_depth(stream, indent);
                    print_fn(stream, indent, it->Value);
                }
                if (value.size() > 1)
                {
                    depth--;
                    indent_depth(stream << '\n', indent);
                }
                stream << ']';
            }
            else
            {
                stream << '[';
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    print_fn(stream, indent, it->Value);
                }
                stream << ']';
            }
        }

        void operator()(const json::Node::Map &value) const
        {
            if (indent)
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
                    print_fn(indent_depth(stream, indent), indent, it->first) << ": ";
                    print_fn(stream, indent, it->second.Value);
                }
                depth--;
                if (!value.empty())
                    indent_depth(stream << '\n', indent);
                stream << '}';
            }
            else
            {
                stream << '{';
                for (auto it = value.begin(); it != value.end(); ++it)
                {
                    if (it != value.begin())
                        stream << ',';
                    print_fn(stream, indent, it->first) << ':';
                    print_fn(stream, indent, it->second.Value);
                }
                stream << '}';
            }
        }

        std::ostream &stream;
        std::size_t indent;
    } visitor{ stream, indent };

    std::visit(visitor, value);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const json::Node &node)
{
    const auto indent = stream.width();
    stream.width(0);

    return node.Print(stream, indent);
}

std::istream &operator>>(std::istream &stream, json::Node &node)
{
    json::Parser parser(stream);
    if (auto exp = parser.Parse())
        node = *exp;
    else
        std::cerr << exp.error() << std::endl;
    return stream;
}
