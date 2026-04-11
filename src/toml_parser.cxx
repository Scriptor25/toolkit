#include <data/utf8.hxx>
#include <toml/parser.hxx>

#include <istream>
#include <limits>

toml::Parser::Parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

toml::Exp<toml::Node> toml::Parser::Parse()
{
    Node node;

    auto table = &node;

    while (m_Buffer >= 0)
    {
        if (SkipWhitespace())
            continue;

        if (SkipComment())
            continue;

        if (SkipEoL())
            continue;

        if (Skip('['))
        {
            const auto is_array = Skip('[');

            SkipWhitespace();

            auto key_exp = ParseKey();
            if (!key_exp)
                return Error("{}", key_exp.error());

            auto key = *std::move(key_exp);

            SkipWhitespace();

            if (!Skip(']'))
                return Error("expected closing bracket");

            if (is_array && !Skip(']'))
                return Error("expected closing bracket");

            auto table_exp = MakeNodeKey(node, key);
            if (!table_exp)
                return Error("{}", table_exp.error());

            table = *std::move(table_exp);

            SkipWhitespace();
            SkipComment();

            if (!SkipEoL())
                return Error("expected end of line");

            if (is_array)
            {
                if (!*table)
                    *table = Node::Vec();

                if (table->Is<Node::Vec>())
                {
                    auto &vec = table->Get<Node::Vec>();
                    table = &vec.emplace_back();
                }
                else
                    return Error("expected vector");
            }

            continue;
        }

        auto key_exp = ParseKey();
        if (!key_exp)
            return Error("{}", key_exp.error());

        auto key = *std::move(key_exp);

        SkipWhitespace();

        if (!Skip('='))
            return Error("expected assignment");

        SkipWhitespace();

        auto value_exp = ParseValue();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        SkipWhitespace();
        SkipComment();

        if (!SkipEoL())
            return Error("expected end of line");

        auto entry_exp = MakeNodeKey(*table, key);
        if (!entry_exp)
            return Error("{}", entry_exp.error());

        auto entry = *std::move(entry_exp);

        *entry = std::move(value);
    }

    return node;
}

toml::Exp<toml::Node> toml::Parser::ParseValue()
{
    switch (Peek())
    {
    case 'f':
        if (!Skip("false"))
            return Error("expected 'false'");
        return false;
    case 't':
        if (!Skip("true"))
            return Error("expected 'true'");
        return true;

    case '+':
    case '-':
    case 'i':
    case 'n':
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
        return ParseNumber();

    case '"':
        return ParseString();

    case '[':
        return ParseArray();

    case '{':
        return ParseTable();

    default:
        return Error("expected value");
    }
}

toml::Exp<toml::Node> toml::Parser::ParseNumber()
{
    std::string buffer;
    auto has_sign = false, is_float = false;
    auto base = 10;

    if (At('+') || At('-'))
    {
        has_sign = true;
        buffer += Pop();
    }

    if (At('i'))
    {
        if (!Skip("inf"))
            return Error("expected 'inf'");
        if (has_sign && buffer.front() == '-')
            return -std::numeric_limits<FloatingPoint>::infinity();
        return std::numeric_limits<FloatingPoint>::infinity();
    }

    if (At('n'))
    {
        if (!Skip("nan"))
            return Error("expected 'nan'");
        if (has_sign && buffer.front() == '-')
            return -std::numeric_limits<FloatingPoint>::quiet_NaN();
        return std::numeric_limits<FloatingPoint>::quiet_NaN();
    }

    if (Skip('0'))
    {
        if (has_sign || At('.'))
        {
            is_float = true;
            buffer += '0';
            buffer += Pop();
        }
        else
        {
            switch (Peek())
            {
            case 'b':
                base = 2;
                break;

            case 'o':
                base = 8;
                break;

            case 'x':
                base = 16;
                break;

            default:
                return 0LL;
            }

            Pop();
        }
    }

    while (AtDigit(base))
    {
        buffer += Pop();

        if (Skip('_'))
            continue;

        if (!is_float && base == 10 && At('.'))
        {
            is_float = true;
            buffer += Pop();
        }
    }

    if (base == 10 && (At('e') || At('E')))
    {
        is_float = true;
        buffer += Pop();

        if (At('+') || At('-'))
            buffer += Pop();

        if (!AtDigit(10))
            return Error("expected base 10 digit");

        do
            buffer += Pop();
        while (AtDigit(10));
    }

    if (is_float)
        return std::stold(buffer);
    return std::stoll(buffer, nullptr, base);
}

toml::Exp<toml::Node> toml::Parser::ParseString()
{
    std::u32string value;

    if (!Skip('"'))
        return Error("expected quote");

    while (!Skip('"'))
    {
        if (!Skip('\\'))
        {
            value += Pop();
            continue;
        }

        switch (Pop())
        {
        case 'b':
            value += static_cast<char32_t>(0x0008);
            break;
        case 't':
            value += static_cast<char32_t>(0x0009);
            break;
        case 'n':
            value += static_cast<char32_t>(0x000A);
            break;
        case 'f':
            value += static_cast<char32_t>(0x000C);
            break;
        case 'r':
            value += static_cast<char32_t>(0x000D);
            break;
        case 'e':
            value += static_cast<char32_t>(0x001B);
            break;
        case '"':
            value += static_cast<char32_t>(0x0022);
            break;
        case '\\':
            value += static_cast<char32_t>(0x005C);
            break;
        case 'x':
        {
            const auto x0 = PopByte();
            if (!x0)
                return Error("{}", x0.error());

            value.push_back(*x0 & 0xff);
            break;
        }
        case 'u':
        {
            const auto x0 = PopByte();
            if (!x0)
                return Error("{}", x0.error());

            const auto x1 = PopByte();
            if (!x1)
                return Error("{}", x1.error());

            value.push_back((*x0 & 0xff) << 8 | *x1 & 0xff);
            break;
        }
        case 'U':
        {
            const auto x0 = PopByte();
            if (!x0)
                return Error("{}", x0.error());

            const auto x1 = PopByte();
            if (!x1)
                return Error("{}", x1.error());

            const auto x2 = PopByte();
            if (!x2)
                return Error("{}", x2.error());

            const auto x3 = PopByte();
            if (!x3)
                return Error("{}", x3.error());

            value.push_back((*x0 & 0xff) << 24 | (*x1 & 0xff) << 16 | (*x2 & 0xff) << 8 | *x3 & 0xff);
            break;
        }
        default:
            return Error("expected escape sequence");
        }
    }

    return data::utf8::encode(value);
}

toml::Exp<toml::Node> toml::Parser::ParseArray()
{
    Node::Vec nodes;

    if (!Skip('['))
        return Error("expected opening bracket");

    while (!At(']'))
    {
        if (SkipWhitespace() || SkipEoL())
            continue;

        auto value_exp = ParseValue();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        nodes.push_back(std::move(value));

        SkipWhitespace() || SkipEoL();

        if (!At(']') && !Skip(','))
            return Error("expected separator");
    }

    if (!Skip(']'))
        return Error("expected closing bracket");

    return nodes;
}

toml::Exp<toml::Node> toml::Parser::ParseTable()
{
    Node::Map nodes;

    if (!Skip('{'))
        return Error("expected opening brace");

    while (!At('}'))
    {
        if (SkipWhitespace() || SkipEoL())
            continue;

        auto key_exp = ParseKey();
        if (!key_exp)
            return Error("{}", key_exp.error());

        auto key = *std::move(key_exp);

        SkipWhitespace();

        if (!Skip('='))
            return Error("expected assignment");

        SkipWhitespace();

        auto value_exp = ParseValue();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        auto entry_exp = MakeNodeKey(nodes, key);
        if (!entry_exp)
            return Error("{}", entry_exp.error());

        auto entry = *std::move(entry_exp);

        *entry = std::move(value);

        SkipWhitespace() || SkipEoL();

        if (!At('}') && !Skip(','))
            return Error("expected separator");
    }

    if (!Skip('}'))
        return Error("expected closing brace");

    return nodes;
}

toml::Exp<toml::Parser::Key> toml::Parser::ParseKey()
{
    Key key;

    do
    {
        if (At('"'))
        {
            auto value_exp = ParseString();
            if (!value_exp)
                return Error("{}", value_exp.error());

            auto value = *std::move(value_exp);

            key.push_back(value.Get<String>());
            SkipWhitespace();
            continue;
        }

        if (!AtKey())
            return Error("expected key symbol");

        std::string buffer;
        do
            buffer += Pop();
        while (AtKey());

        key.push_back(std::move(buffer));
        SkipWhitespace();
    }
    while (Skip('.'));

    return key;
}

toml::Exp<toml::Node *> toml::Parser::MakeNodeKey(Node &node, const Key &key)
{
    auto ptr = &node;
    for (auto &k : key)
    {
        if (!*ptr)
            *ptr = Node::Map();

        if (ptr->Is<Node::Map>())
            ptr = &(*ptr)[k];
        else
            return Error("expected map");
    }
    return ptr;
}

toml::Exp<toml::Node *> toml::Parser::MakeNodeKey(Node::Map &nodes, const Key &key)
{
    if (key.size() == 1)
        return &nodes[key.front()];
    return MakeNodeKey(nodes[key.front()], { key.begin() + 1, key.end() });
}

void toml::Parser::Get()
{
    m_Buffer = m_Stream.get();
}

char toml::Parser::Pop()
{
    const auto buffer = m_Buffer;
    m_Buffer = m_Stream.get();
    return static_cast<char>(buffer);
}

char toml::Parser::Peek() const
{
    return static_cast<char>(m_Buffer);
}

toml::Exp<unsigned char> toml::Parser::PopHalfByte()
{
    const auto c = Pop();
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return Error("expected base 16 digit");
}

toml::Exp<unsigned char> toml::Parser::PopByte()
{
    const auto hi = PopHalfByte();
    if (!hi)
        return hi;

    const auto lo = PopHalfByte();
    if (!lo)
        return lo;

    return (*hi & 0xF) << 4 | *lo & 0xF;
}

bool toml::Parser::At(const int c) const
{
    return m_Buffer == c;
}

bool toml::Parser::AtKey() const
{
    return ('0' <= m_Buffer && m_Buffer <= '9')
           || ('A' <= m_Buffer && m_Buffer <= 'Z')
           || ('a' <= m_Buffer && m_Buffer <= 'z')
           || m_Buffer == '_'
           || m_Buffer == '-';
}

bool toml::Parser::AtDigit(const int base) const
{
    switch (base)
    {
    case 2:
        return '0' <= m_Buffer && m_Buffer <= '1';

    case 8:
        return '0' <= m_Buffer && m_Buffer <= '7';

    case 10:
        return '0' <= m_Buffer && m_Buffer <= '9';

    case 16:
        return ('0' <= m_Buffer && m_Buffer <= '9')
               || ('A' <= m_Buffer && m_Buffer <= 'F')
               || ('a' <= m_Buffer && m_Buffer <= 'f');

    default:
        return false;
    }
}

bool toml::Parser::Skip(const char c)
{
    const auto skip = m_Buffer == c;
    if (skip)
        Get();
    return skip;
}

bool toml::Parser::Skip(const std::string_view s)
{
    for (const auto c : s)
        if (!Skip(c))
            return false;
    return true;
}

bool toml::Parser::SkipWhitespace()
{
    if (m_Buffer != ' ' && m_Buffer != '\t')
        return false;

    do
        Get();
    while (m_Buffer == ' ' || m_Buffer == '\t');

    return true;
}

bool toml::Parser::SkipComment()
{
    if (m_Buffer != '#')
        return false;

    do
        Pop();
    while (m_Buffer != '\n' && m_Buffer != '\r');

    return true;
}

bool toml::Parser::SkipEoL()
{
    if (m_Buffer != '\n' && m_Buffer != '\r' && m_Buffer >= 0)
        return false;

    do
        Get();
    while (m_Buffer == '\n' || m_Buffer == '\r');

    return true;
}
