#include <toolkit/utf8.hxx>

#include <json/parser.hxx>

#include <istream>

json::Parser::Parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

toolkit::result<json::Node> json::Parser::Parse()
{
    toolkit::result<Node> exp;

    SkipWhitespace();

    switch (m_Buffer)
    {
    case 'n':
        if (Skip("null"))
        {
            exp = nullptr;
        }
        else
        {
            exp = toolkit::make_error("expected 'null'");
        }
        break;
    case 'f':
        if (Skip("false"))
        {
            exp = false;
        }
        else
        {
            exp = toolkit::make_error("expected 'false'");
        }
        break;
    case 't':
        if (Skip("true"))
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
        exp = ParseNumber();
        break;
    case '"':
        exp = ParseString();
        break;
    case '[':
        exp = ParseArray();
        break;
    case '{':
        exp = ParseObject();
        break;
    default:
        break;
    }

    SkipWhitespace();

    return exp;
}

toolkit::result<json::Node> json::Parser::ParseNumber()
{
    std::string buffer;
    auto is_float = false;

    if (At('-'))
    {
        buffer += Pop();
    }

    if (At('0'))
    {
        buffer += Pop();
    }
    else if ('1' <= m_Buffer && m_Buffer <= '9')
    {
        do
        {
            buffer += Pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }
    else
    {
        return toolkit::make_error("expected base 10 digit");
    }

    if (At('.'))
    {
        buffer += Pop();
        is_float = true;

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
        {
            return toolkit::make_error("expected base 10 digit");
        }

        do
        {
            buffer += Pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (At('e') || At('E'))
    {
        buffer += Pop();
        is_float = true;

        if (At('-') || At('+'))
        {
            buffer += Pop();
        }

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
        {
            return toolkit::make_error("expected base 10 digit");
        }

        do
        {
            buffer += Pop();
        }
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (is_float)
    {
        return { std::stold(buffer) };
    }

    return { std::stoll(buffer) };
}

toolkit::result<json::Node> json::Parser::ParseString()
{
    std::u32string value;

    if (!Skip('"'))
    {
        return toolkit::make_error("expected quote");
    }

    while (!Skip('"'))
    {
        if (!Skip('\\'))
        {
            value += Pop();
            continue;
        }

        switch (Pop())
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
            const auto hi = PopByte();
            if (!hi)
            {
                return toolkit::make_error("{}", hi.error());
            }

            const auto lo = PopByte();
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

toolkit::result<json::Node> json::Parser::ParseArray()
{
    Array nodes;

    if (!Skip('['))
    {
        return toolkit::make_error("expected opening bracket");
    }

    SkipWhitespace();

    if (!Skip(']'))
    {
        do
        {
            auto element = Parse();
            if (!element)
            {
                return element;
            }

            nodes.push_back(std::move(*element));
        }
        while (Skip(','));

        if (!Skip(']'))
        {
            return toolkit::make_error("expected closing bracket");
        }
    }

    return { std::move(nodes) };
}

toolkit::result<json::Node> json::Parser::ParseObject()
{
    Object nodes;

    if (!Skip('{'))
    {
        return toolkit::make_error("expected opening brace");
    }

    SkipWhitespace();

    if (!Skip('}'))
    {
        do
        {
            SkipWhitespace();

            auto key = ParseString();
            if (!key)
            {
                return key;
            }

            SkipWhitespace();

            if (!Skip(':'))
            {
                return toolkit::make_error("expected colon");
            }

            auto value = Parse();
            if (!value)
            {
                return value;
            }

            nodes[key->Get<String>()] = std::move(*value);
        }
        while (Skip(','));

        if (!Skip('}'))
        {
            return toolkit::make_error("expected closing brace");
        }
    }

    return { std::move(nodes) };
}

void json::Parser::Get()
{
    m_Buffer = m_Stream.get();
}

char json::Parser::Pop()
{
    const auto buffer = m_Buffer;
    m_Buffer = m_Stream.get();
    return static_cast<char>(buffer);
}

toolkit::result<uint8_t> json::Parser::PopHalfByte()
{
    const auto c = Pop();
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

toolkit::result<uint8_t> json::Parser::PopByte()
{
    auto hi = PopHalfByte();
    if (!hi)
    {
        return hi;
    }

    auto lo = PopHalfByte();
    if (!lo)
    {
        return lo;
    }

    return (*hi & 0xF) << 4 | *lo & 0xF;
}

bool json::Parser::At(const char c) const
{
    return m_Buffer == c;
}

bool json::Parser::Skip(const char c)
{
    const auto skip = m_Buffer == c;
    if (skip)
    {
        Get();
    }
    return skip;
}

bool json::Parser::Skip(const std::string_view s)
{
    for (const auto c : s)
    {
        if (!Skip(c))
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

bool json::Parser::SkipWhitespace()
{
    if (!is_whitespace(m_Buffer))
    {
        return false;
    }

    do
    {
        Get();
    }
    while (is_whitespace(m_Buffer));

    return true;
}
