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
    if (!Skip('n'))
        return {};
    if (!Skip('u'))
        return {};
    if (!Skip('l'))
        return {};
    if (!Skip('l'))
        return {};
    return { nullptr };
}

json::Node json::Parser::ParseBoolean()
{
    if (Skip('f'))
    {
        if (!Skip('a'))
            return {};
        if (!Skip('l'))
            return {};
        if (!Skip('s'))
            return {};
        if (!Skip('e'))
            return {};
        return { false };
    }

    if (!Skip('t'))
        return {};
    if (!Skip('r'))
        return {};
    if (!Skip('u'))
        return {};
    if (!Skip('e'))
        return {};
    return { true };
}

json::Node json::Parser::ParseNumber()
{
    std::string buffer;

    if (Skip('-'))
        buffer += '-';

    if (Skip('0'))
    {
        buffer += '0';
    }
    else if ('1' <= m_Buffer && m_Buffer <= '9')
    {
        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }
    else
        return {};

    if (Skip('.'))
    {
        buffer += '.';

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
            return {};

        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    if (m_Buffer == 'e' || m_Buffer == 'E')
    {
        buffer += static_cast<char>(m_Buffer);
        Get();

        if (m_Buffer == '-' || m_Buffer == '+')
            buffer += Pop();

        if (!('0' <= m_Buffer && m_Buffer <= '9'))
            return {};

        do
            buffer += Pop();
        while ('0' <= m_Buffer && m_Buffer <= '9');
    }

    return { std::stold(buffer, nullptr) };
}

json::Node json::Parser::ParseString()
{
    std::string value;

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

    return { std::move(value) };
}

json::Node json::Parser::ParseArray()
{
    std::vector<Node> elements;

    if (!Skip('['))
        return {};

    SkipWhitespace();

    if (!Skip(']'))
    {
        do
        {
            auto element = Parse();
            if (element.IsUndefined())
                return {};

            elements.push_back(std::move(element));
        }
        while (Skip(','));

        if (!Skip(']'))
            return {};
    }

    return { std::move(elements) };
}

json::Node json::Parser::ParseObject()
{
    std::map<std::string, Node> elements;

    if (!Skip('{'))
        return {};

    SkipWhitespace();

    if (!Skip('}'))
    {
        do
        {
            SkipWhitespace();

            const auto key = ParseString();
            if (key.IsUndefined())
                return {};

            SkipWhitespace();

            if (!Skip(':'))
                return {};

            auto value = Parse();
            if (value.IsUndefined())
                return {};

            elements[key.GetString()] = std::move(value);
        }
        while (Skip(','));

        if (!Skip('}'))
            return {};
    }

    return { std::move(elements) };
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
