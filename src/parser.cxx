#include <json/json.hxx>
#include <json/parser.hxx>
#include <json/utf8.hxx>

json::Parser::Parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

json::Node json::Parser::Parse()
{
    Node node;

    SkipWhitespace();

    switch (m_Buffer)
    {
    case 'n':
        node = ParseNull();
        break;
    case 'f':
    case 't':
        node = ParseBoolean();
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
        node = ParseNumber();
        break;
    case '"':
        node = ParseString();
        break;
    case '[':
        node = ParseArray();
        break;
    case '{':
        node = ParseObject();
        break;
    default:
        break;
    }

    SkipWhitespace();

    return node;
}

json::Node json::Parser::ParseNull()
{
    if (!Skip("null"))
        return {};
    return Node(nullptr);
}

json::Node json::Parser::ParseBoolean()
{
    if (At('f'))
    {
        if (!Skip("false"))
            return {};
        return Node(false);
    }

    if (!Skip("true"))
        return {};
    return Node(true);
}

json::Node json::Parser::ParseNumber()
{
    std::string buffer;

    if (At('-'))
        buffer += Pop();

    if (At('0'))
    {
        buffer += Pop();
    }
    else if ('1' <= m_Buffer && m_Buffer <= '9')
    {
        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }
    else
        return {};

    if (At('.'))
    {
        buffer += Pop();

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
            return {};

        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (At('e') || At('E'))
    {
        buffer += static_cast<char>(m_Buffer);
        Get();

        if (At('-') || At('+'))
            buffer += Pop();

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
            return {};

        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    return Node(std::stold(buffer, nullptr));
}

json::Node json::Parser::ParseString()
{
    String value;

    if (!Skip('"'))
        return {};

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
            char buffer[5];
            buffer[0] = Pop();
            buffer[1] = Pop();
            buffer[2] = Pop();
            buffer[3] = Pop();
            buffer[4] = 0;

            char out[4];
            const auto len = utf8::encode(std::stoi(buffer, nullptr, 0x10), out);
            value.append(out, out + len);
            break;
        }
        default:
            return {};
        }
    }

    return Node(std::move(value));
}

json::Node json::Parser::ParseArray()
{
    Array nodes;

    if (!Skip('['))
        return {};

    SkipWhitespace();

    if (!Skip(']'))
    {
        do
        {
            auto element = Parse();
            if (!element)
                return {};

            nodes.push_back(std::move(element));
        }
        while (Skip(','));

        if (!Skip(']'))
            return {};
    }

    return Node(std::move(nodes));
}

json::Node json::Parser::ParseObject()
{
    Object nodes;

    if (!Skip('{'))
        return {};

    SkipWhitespace();

    if (!Skip('}'))
    {
        do
        {
            SkipWhitespace();

            const auto key = ParseString();
            if (!key)
                return {};

            SkipWhitespace();

            if (!Skip(':'))
                return {};

            auto value = Parse();
            if (!value)
                return {};

            nodes[key.Get<String>()] = std::move(value);
        }
        while (Skip(','));

        if (!Skip('}'))
            return {};
    }

    return Node(std::move(nodes));
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

bool json::Parser::At(const char c) const
{
    return m_Buffer == c;
}

bool json::Parser::Skip(const char c)
{
    const auto skip = m_Buffer == c;
    if (skip)
        Get();
    return skip;
}

bool json::Parser::Skip(const std::string_view s)
{
    for (const auto c : s)
        if (!Skip(c))
            return false;
    return true;
}

static bool is_whitespace(const int c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool json::Parser::SkipWhitespace()
{
    if (!is_whitespace(m_Buffer))
        return false;

    do
        Get();
    while (is_whitespace(m_Buffer));

    return true;
}
