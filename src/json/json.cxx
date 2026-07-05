#include <json/parser.hxx>

#include <toolkit/utf8.hxx>

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

static std::ostream &print_fn(std::ostream &stream, const unsigned indent, const json::node_t::value_type &value)
{
    struct
    {
        void operator()(json::undefined_t) const
        {
            stream << "<undefined>";
        }

        void operator()(json::null_t) const
        {
            stream << "null";
        }

        void operator()(const json::boolean_t value) const
        {
            stream << (value ? "true" : "false");
        }

        void operator()(const json::integer_t value) const
        {
            stream << value;
        }

        void operator()(const json::floating_point_t value) const
        {
            const auto flags = stream.flags();

            stream << std::scientific << value;

            stream.flags(flags);
        }

        void operator()(const json::string_t &value) const
        {
            stream << '"';

            for (const auto c : toolkit::utf8::decode(value))
            {
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
                    {
                        stream << static_cast<char>(c);
                    }
                    else
                    {
                        const auto flags = stream.flags();

                        stream
                                << "\\u"
                                << std::setw(4)
                                << std::setfill('0')
                                << std::hex
                                << static_cast<int>(c);

                        stream.flags(flags);
                    }
                    break;
                }
            }

            stream << '"';
        }

        void operator()(const json::array_t &value) const
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

                auto first = true;
                for (auto &it : value)
                {
                    if (!it)
                    {
                        continue;
                    }

                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        stream << ',' << '\n';
                    }

                    if (value.size() > 1)
                    {
                        indent_depth(stream, indent);
                    }

                    print_fn(stream, indent, *it);
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

                auto first = true;
                for (auto &it : value)
                {
                    if (!it)
                    {
                        continue;
                    }

                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        stream << ',';
                    }

                    print_fn(stream, indent, *it);
                }

                stream << ']';
            }
        }

        void operator()(const json::object_t &value) const
        {
            if (indent)
            {
                auto &depth = get_context_depth(stream);

                stream << '{';

                if (!value.empty())
                {
                    stream << '\n';
                }

                depth++;

                auto first = true;
                for (const auto &[key_, val_] : value)
                {
                    if (!val_)
                    {
                        continue;
                    }

                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        stream << ',' << '\n';
                    }

                    print_fn(indent_depth(stream, indent), indent, key_) << ": ";
                    print_fn(stream, indent, *val_);
                }

                depth--;

                if (!value.empty())
                {
                    indent_depth(stream << '\n', indent);
                }

                stream << '}';
            }
            else
            {
                stream << '{';

                auto first = true;
                for (const auto &[key_, val_] : value)
                {
                    if (!val_)
                    {
                        continue;
                    }

                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        stream << ',';
                    }

                    print_fn(stream, indent, key_) << ':';
                    print_fn(stream, indent, *val_);
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

std::ostream &data::node_traits<
    json::null_t,
    json::boolean_t,
    json::integer_t,
    json::floating_point_t,
    json::string_t
>::print(std::ostream &stream, const json::node_t &node)
{
    const auto indent = stream.width();

    stream.width(0);

    return print_fn(stream, indent, *node);
}

std::istream &data::node_traits<
    json::null_t,
    json::boolean_t,
    json::integer_t,
    json::floating_point_t,
    json::string_t
>::parse(std::istream &stream, json::node_t &node)
{
    json::parser parser(stream);
    if (auto result = parser.parse())
        node = *std::move(result);
    else
        std::cerr << result.error() << std::endl;
    return stream;
}
