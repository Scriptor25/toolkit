#include <xml/parser.hxx>
#include <xml/xml.hxx>

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

static std::ostream &print_attribute_fn(
    std::ostream &stream,
    const std::string &name,
    const xml::node::value_type &value)
{
    // attributes:
    //  - boolean:  {name}
    //  - string:   {name}="{value}"

    struct
    {
        void operator()(xml::undefined) const
        {
            // noop
        }

        void operator()(const xml::boolean value) const
        {
            if (value)
                stream << ' ' << name;
        }

        void operator()(const xml::string &value) const
        {
            // TODO: escape string
            stream << ' ' << name << "=\"" << value << '"';
        }

        void operator()(const xml::elements &) const
        {
            throw std::runtime_error("invalid attribute value");
        }

        void operator()(const xml::element &) const
        {
            throw std::runtime_error("invalid attribute value");
        }

        std::ostream &stream;
        const std::string &name;
    } visitor
    {
        .stream = stream,
        .name = name,
    };

    std::visit(visitor, value);

    return stream;
}

static std::ostream &print_fn(std::ostream &stream, const unsigned indent, const xml::node::value_type &value)
{
    struct
    {
        void operator()(xml::undefined) const
        {
            // noop
        }

        void operator()(const xml::boolean value) const
        {
            stream << (value ? "true" : "false");
        }

        void operator()(const xml::string &value) const
        {
            // TODO: escape string
            stream << value;
        }

        void operator()(const xml::elements &value) const
        {
            if (indent)
            {
                auto &depth = get_context_depth(stream);

                if (!value.empty())
                {
                    stream << '\n';
                    ++depth;
                }

                for (const auto &it : value)
                {
                    indent_depth(stream, indent);

                    print_fn(stream, indent, *it);
                }

                if (!value.empty())
                {
                    --depth;
                    indent_depth(stream << '\n', indent);
                }
            }
            else
            {
                for (const auto &it : value)
                    print_fn(stream, indent, *it);
            }
        }

        void operator()(const xml::element &value) const
        {
            const auto &tag = value.at("tag");
            const auto &attributes = value.at("attributes");
            const auto &elements = value.at("elements");

            stream << '<' << tag;

            for (const auto &[name, attribute] : attributes)
                print_attribute_fn(stream, name, *attribute);

            if (elements.empty())
                stream << " />";
            else
            {
                stream << '>';
                print_fn(stream, indent, *elements);
                stream << "</" << tag << '>';
            }
        }

        std::ostream &stream;
        unsigned indent;
    } visitor
    {
        .stream = stream,
        .indent = indent,
    };

    std::visit(visitor, value);

    return stream;
}

std::ostream &data::node_traits<bool, std::string>::print(std::ostream &stream, const xml::node &node)
{
    const auto indent = stream.width();

    stream.width(0);

    return print_fn(stream, indent, *node);
}

std::istream &data::node_traits<bool, std::string>::parse(std::istream &stream, xml::node &node)
{
    xml::parser parser(stream);
    if (auto result = parser.parse())
        node = *std::move(result);
    else
        std::cerr << result.error() << std::endl;
    return stream;
}
