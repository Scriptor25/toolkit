#include <toml/parser.hxx>

#include <toolkit/utf8.hxx>

#include <istream>
#include <limits>

toml::parser::parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

toolkit::result<toml::node> toml::parser::parse()
{
    node root;

    auto table = &root;

    while (m_Buffer >= 0)
    {
        if (skip_whitespace())
            continue;

        if (skip_comment())
            continue;

        if (skip_eol())
            continue;

        if (skip('['))
        {
            const auto is_array = skip('[');

            skip_whitespace();

            auto key_exp = parse_key();
            if (!key_exp)
                return toolkit::make_error("{}", key_exp.error());

            auto key = *std::move(key_exp);

            skip_whitespace();

            if (!skip(']'))
                return toolkit::make_error("expected closing bracket");

            if (is_array && !skip(']'))
                return toolkit::make_error("expected closing bracket");

            auto table_exp = find_node(root, key);
            if (!table_exp)
                return toolkit::make_error("{}", table_exp.error());

            table = *std::move(table_exp);

            skip_whitespace();
            skip_comment();

            if (!skip_eol())
                return toolkit::make_error("expected end of line");

            if (is_array)
            {
                if (!*table)
                    *table = array();

                if (table->is<array>())
                {
                    auto &vec = table->get<array>();
                    table = &vec.emplace_back();
                }
                else
                    return toolkit::make_error("expected vector");
            }

            continue;
        }

        auto key_exp = parse_key();
        if (!key_exp)
            return toolkit::make_error("{}", key_exp.error());

        auto key = *std::move(key_exp);

        skip_whitespace();

        if (!skip('='))
            return toolkit::make_error("expected assignment");

        skip_whitespace();

        auto value_exp = parse_value();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        skip_whitespace();
        skip_comment();

        if (!skip_eol())
            return toolkit::make_error("expected end of line");

        auto entry_exp = find_node(*table, key);
        if (!entry_exp)
            return toolkit::make_error("{}", entry_exp.error());

        auto entry = *std::move(entry_exp);

        *entry = std::move(value);
    }

    return root;
}

toolkit::result<toml::node> toml::parser::parse_value()
{
    switch (peek())
    {
    case 'f':
        if (!skip("false"))
            return toolkit::make_error("expected 'false'");
        return { false };
    case 't':
        if (!skip("true"))
            return toolkit::make_error("expected 'true'");
        return { true };

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
        return parse_number();

    case '"':
        return parse_string();

    case '[':
        return parse_array();

    case '{':
        return parse_table();

    default:
        return toolkit::make_error("expected value");
    }
}

toolkit::result<toml::node> toml::parser::parse_number()
{
    std::string buffer;
    auto has_sign = false, is_float = false;
    auto base = 10;

    if (at('+') || at('-'))
    {
        has_sign = true;
        buffer += pop();
    }

    if (at('i'))
    {
        if (!skip("inf"))
            return toolkit::make_error("expected 'inf'");
        if (has_sign && buffer.front() == '-')
            return { -std::numeric_limits<floating_point>::infinity() };
        return { std::numeric_limits<floating_point>::infinity() };
    }

    if (at('n'))
    {
        if (!skip("nan"))
            return toolkit::make_error("expected 'nan'");
        if (has_sign && buffer.front() == '-')
            return { -std::numeric_limits<floating_point>::quiet_NaN() };
        return { std::numeric_limits<floating_point>::quiet_NaN() };
    }

    if (skip('0'))
    {
        if (has_sign || at('.'))
        {
            is_float = true;
            buffer += '0';
            buffer += pop();
        }
        else
        {
            switch (peek())
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
                return { 0ull };
            }

            pop();
        }
    }

    while (at_digit(base))
    {
        buffer += pop();

        if (skip('_'))
            continue;

        if (!is_float && base == 10 && at('.'))
        {
            is_float = true;
            buffer += pop();
        }
    }

    if (base == 10 && (at('e') || at('E')))
    {
        is_float = true;
        buffer += pop();

        if (at('+') || at('-'))
            buffer += pop();

        if (!at_digit(10))
            return toolkit::make_error("expected base 10 digit");

        do
            buffer += pop();
        while (at_digit(10));
    }

    if (is_float)
        return { std::stold(buffer) };

    return { std::stoll(buffer, nullptr, base) };
}

toolkit::result<toml::node> toml::parser::parse_string()
{
    std::u32string value;

    if (!skip('"'))
        return toolkit::make_error("expected quote");

    while (!skip('"'))
    {
        if (!skip('\\'))
        {
            value += pop();
            continue;
        }

        switch (pop())
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
            const auto x0 = pop_byte();
            if (!x0)
                return toolkit::make_error("{}", x0.error());

            value.push_back(*x0 & 0xff);
            break;
        }
        case 'u':
        {
            const auto x0 = pop_byte();
            if (!x0)
                return toolkit::make_error("{}", x0.error());

            const auto x1 = pop_byte();
            if (!x1)
                return toolkit::make_error("{}", x1.error());

            value.push_back((*x0 & 0xff) << 8 | *x1 & 0xff);
            break;
        }
        case 'U':
        {
            const auto x0 = pop_byte();
            if (!x0)
                return toolkit::make_error("{}", x0.error());

            const auto x1 = pop_byte();
            if (!x1)
                return toolkit::make_error("{}", x1.error());

            const auto x2 = pop_byte();
            if (!x2)
                return toolkit::make_error("{}", x2.error());

            const auto x3 = pop_byte();
            if (!x3)
                return toolkit::make_error("{}", x3.error());

            value.push_back((*x0 & 0xff) << 24 | (*x1 & 0xff) << 16 | (*x2 & 0xff) << 8 | *x3 & 0xff);
            break;
        }
        default:
            return toolkit::make_error("expected escape sequence");
        }
    }

    return { toolkit::utf8::encode(std::move(value)) };
}

toolkit::result<toml::node> toml::parser::parse_array()
{
    array nodes;

    if (!skip('['))
        return toolkit::make_error("expected opening bracket");

    while (!at(']'))
    {
        if (skip_whitespace() || skip_eol())
            continue;

        auto value_exp = parse_value();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        nodes.push_back(std::move(value));

        skip_whitespace() || skip_eol();

        if (!at(']') && !skip(','))
            return toolkit::make_error("expected separator");
    }

    if (!skip(']'))
        return toolkit::make_error("expected closing bracket");

    return { std::move(nodes) };
}

toolkit::result<toml::node> toml::parser::parse_table()
{
    table nodes;

    if (!skip('{'))
        return toolkit::make_error("expected opening brace");

    while (!at('}'))
    {
        if (skip_whitespace() || skip_eol())
            continue;

        auto key_exp = parse_key();
        if (!key_exp)
            return toolkit::make_error("{}", key_exp.error());

        auto key = *std::move(key_exp);

        skip_whitespace();

        if (!skip('='))
            return toolkit::make_error("expected assignment");

        skip_whitespace();

        auto value_exp = parse_value();
        if (!value_exp)
            return value_exp;

        auto value = *std::move(value_exp);

        auto entry_exp = find_node(nodes, key);
        if (!entry_exp)
            return toolkit::make_error("{}", entry_exp.error());

        auto entry = *std::move(entry_exp);

        *entry = std::move(value);

        skip_whitespace() || skip_eol();

        if (!at('}') && !skip(','))
            return toolkit::make_error("expected separator");
    }

    if (!skip('}'))
        return toolkit::make_error("expected closing brace");

    return { std::move(nodes) };
}

toolkit::result<toml::parser::key_t> toml::parser::parse_key()
{
    key_t key;

    do
    {
        if (at('"'))
        {
            auto value_exp = parse_string();
            if (!value_exp)
                return toolkit::make_error("{}", value_exp.error());

            auto value = *std::move(value_exp);

            key.push_back(value.get<string>());
            skip_whitespace();
            continue;
        }

        if (!at_key())
            return toolkit::make_error("expected key symbol");

        std::string buffer;
        do
            buffer += pop();
        while (at_key());

        key.push_back(std::move(buffer));
        skip_whitespace();
    }
    while (skip('.'));

    return key;
}

toolkit::result<toml::node *> toml::parser::find_node(node &root, const key_t &key)
{
    auto ptr = &root;
    for (auto &k : key)
    {
        if (!*ptr)
            *ptr = table();

        if (ptr->is<table>())
            ptr = &(*ptr)[k];
        else
            return toolkit::make_error("expected map");
    }
    return ptr;
}

toolkit::result<toml::node *> toml::parser::find_node(table &root, const key_t &key)
{
    if (key.size() == 1)
        return &root[key.front()];
    return find_node(root[key.front()], { key.begin() + 1, key.end() });
}

void toml::parser::get()
{
    m_Buffer = m_Stream.get();
}

char toml::parser::pop()
{
    const auto buffer = m_Buffer;
    m_Buffer = m_Stream.get();
    return static_cast<char>(buffer);
}

char toml::parser::peek() const
{
    return static_cast<char>(m_Buffer);
}

toolkit::result<uint8_t> toml::parser::pop_nibble()
{
    const auto c = pop();
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return toolkit::make_error("expected base 16 digit");
}

toolkit::result<uint8_t> toml::parser::pop_byte()
{
    auto hi = pop_nibble();
    if (!hi)
        return hi;

    auto lo = pop_nibble();
    if (!lo)
        return lo;

    return (*hi & 0xF) << 4 | *lo & 0xF;
}

bool toml::parser::at(const int c) const
{
    return m_Buffer == c;
}

bool toml::parser::at_key() const
{
    return ('0' <= m_Buffer && m_Buffer <= '9')
           || ('A' <= m_Buffer && m_Buffer <= 'Z')
           || ('a' <= m_Buffer && m_Buffer <= 'z')
           || m_Buffer == '_'
           || m_Buffer == '-';
}

bool toml::parser::at_digit(const int base) const
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

bool toml::parser::skip(const char c)
{
    const auto skip = m_Buffer == c;
    if (skip)
        get();
    return skip;
}

bool toml::parser::skip(const std::string_view s)
{
    for (const auto c : s)
        if (!skip(c))
            return false;
    return true;
}

bool toml::parser::skip_whitespace()
{
    if (m_Buffer != ' ' && m_Buffer != '\t')
        return false;

    do
        get();
    while (m_Buffer == ' ' || m_Buffer == '\t');

    return true;
}

bool toml::parser::skip_comment()
{
    if (m_Buffer != '#')
        return false;

    do
        pop();
    while (m_Buffer != '\n' && m_Buffer != '\r');

    return true;
}

bool toml::parser::skip_eol()
{
    if (m_Buffer != '\n' && m_Buffer != '\r' && m_Buffer >= 0)
        return false;

    do
        get();
    while (m_Buffer == '\n' || m_Buffer == '\r');

    return true;
}
