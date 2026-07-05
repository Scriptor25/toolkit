#include <toolkit/utf8.hxx>

#include <json/parser.hxx>

#include <istream>

json::parser::parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

toolkit::result<json::node_t> json::parser::parse()
{
    toolkit::result<node_t> exp;

    skip_whitespace();

    switch (m_Buffer)
    {
    case 'n':
        if (skip("null"))
        {
            exp = nullptr;
        }
        else
        {
            exp = toolkit::make_error("expected 'null'");
        }
        break;
    case 'f':
        if (skip("false"))
        {
            exp = false;
        }
        else
        {
            exp = toolkit::make_error("expected 'false'");
        }
        break;
    case 't':
        if (skip("true"))
        {
            exp = true;
        }
        else
        {
            exp = toolkit::make_error("expected 'true'");
        }
        break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        exp = parse_number();
        break;
    case '"':
        exp = parse_string();
        break;
    case '[':
        exp = parse_array();
        break;
    case '{':
        exp = parse_object();
        break;
    default:
        break;
    }

    skip_whitespace();

    return exp;
}

toolkit::result<json::node_t> json::parser::parse_number()
{
    std::string buffer;
    auto is_float = false;

    if (at('-'))
    {
        buffer += pop();
    }

    if (at('0'))
    {
        buffer += pop();
    }
    else if ('1' <= m_Buffer && m_Buffer <= '9')
    {
        do
        {
            buffer += pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }
    else
    {
        return toolkit::make_error("expected base 10 digit");
    }

    if (at('.'))
    {
        buffer += pop();
        is_float = true;

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
        {
            return toolkit::make_error("expected base 10 digit");
        }

        do
        {
            buffer += pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (at('e') || at('E'))
    {
        buffer += pop();
        is_float = true;

        if (at('-') || at('+'))
        {
            buffer += pop();
        }

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
        {
            return toolkit::make_error("expected base 10 digit");
        }

        do
        {
            buffer += pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (is_float)
    {
        return { std::stold(buffer) };
    }

    return { std::stoll(buffer) };
}

toolkit::result<json::node_t> json::parser::parse_string()
{
    std::u32string value;

    if (!skip('"'))
    {
        return toolkit::make_error("expected quote");
    }

    while (!skip('"'))
    {
        if (!skip('\\'))
        {
            value += pop();
            continue;
        }

        switch (pop())
        {
        case '"':
            value += '"';
            break;
        case '\\':
            value += '\\';
            break;
        case '/':
            value += '/';
            break;
        case 'b':
            value += '\b';
            break;
        case 'f':
            value += '\f';
            break;
        case 'n':
            value += '\n';
            break;
        case 'r':
            value += '\r';
            break;
        case 't':
            value += '\t';
            break;
        case 'u':
        {
            const auto hi = pop_byte();
            if (!hi)
            {
                return toolkit::make_error("{}", hi.error());
            }

            const auto lo = pop_byte();
            if (!lo)
            {
                return toolkit::make_error("{}", lo.error());
            }

            value.push_back((*hi & 0xff) << 8 | *lo & 0xff);
            break;
        }
        default:
            return toolkit::make_error("expected escape sequence");
        }
    }

    return { toolkit::utf8::encode(std::move(value)) };
}

toolkit::result<json::node_t> json::parser::parse_array()
{
    array_t nodes;

    if (!skip('['))
    {
        return toolkit::make_error("expected opening bracket");
    }

    skip_whitespace();

    if (!skip(']'))
    {
        do
        {
            auto element = parse();
            if (!element)
            {
                return element;
            }

            nodes.push_back(std::move(*element));
        }
        while (skip(','));

        if (!skip(']'))
        {
            return toolkit::make_error("expected closing bracket");
        }
    }

    return { std::move(nodes) };
}

toolkit::result<json::node_t> json::parser::parse_object()
{
    object_t nodes;

    if (!skip('{'))
    {
        return toolkit::make_error("expected opening brace");
    }

    skip_whitespace();

    if (!skip('}'))
    {
        do
        {
            skip_whitespace();

            auto key = parse_string();
            if (!key)
            {
                return key;
            }

            skip_whitespace();

            if (!skip(':'))
            {
                return toolkit::make_error("expected colon");
            }

            auto value = parse();
            if (!value)
            {
                return value;
            }

            nodes[key->get<string_t>()] = std::move(*value);
        }
        while (skip(','));

        if (!skip('}'))
        {
            return toolkit::make_error("expected closing brace");
        }
    }

    return { std::move(nodes) };
}

void json::parser::get()
{
    m_Buffer = m_Stream.get();
}

char json::parser::pop()
{
    const auto buffer = m_Buffer;
    m_Buffer = m_Stream.get();
    return static_cast<char>(buffer);
}

toolkit::result<uint8_t> json::parser::pop_nibble()
{
    const auto c = pop();
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    if ('A' <= c && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if ('a' <= c && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return toolkit::make_error("expected base 16 digit");
}

toolkit::result<uint8_t> json::parser::pop_byte()
{
    auto hi = pop_nibble();
    if (!hi)
    {
        return hi;
    }

    auto lo = pop_nibble();
    if (!lo)
    {
        return lo;
    }

    return (*hi & 0xF) << 4 | *lo & 0xF;
}

bool json::parser::at(const char c) const
{
    return m_Buffer == c;
}

bool json::parser::skip(const char c)
{
    const auto skip = m_Buffer == c;
    if (skip)
    {
        get();
    }
    return skip;
}

bool json::parser::skip(const std::string_view s)
{
    for (const auto c : s)
    {
        if (!skip(c))
        {
            return false;
        }
    }
    return true;
}

static bool is_whitespace(const int c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool json::parser::skip_whitespace()
{
    if (!is_whitespace(m_Buffer))
    {
        return false;
    }

    do
    {
        get();
    }
    while (is_whitespace(m_Buffer));

    return true;
}
