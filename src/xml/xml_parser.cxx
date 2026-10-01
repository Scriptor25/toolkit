#include <xml/parser.hxx>

#include <istream>

static bool is_whitespace(const int c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

xml::parser::parser(std::istream &stream)
    : m_Stream(stream),
      m_Buffer(stream.get())
{
}

toolkit::result<xml::node> xml::parser::parse()
{
    return parse_document();
}

toolkit::result<xml::node> xml::parser::parse_document()
{
    bool skip_start{};
    while (true)
    {
        if (skip_whitespace())
            continue;

        if (skip('<'))
        {
            // <? ... ?>
            if (skip('?'))
            {
                std::unordered_map<std::string, node> attributes;
                while (!skip('?'))
                {
                    if (skip_whitespace())
                        continue;

                    std::string name;
                    while (!(is_whitespace(m_Buffer) || at('=') || at('?')))
                        name += pop();

                    if (auto res = parse_attribute() >> attributes[name]; !res)
                        return res;
                }

                if (!skip('>'))
                    return toolkit::make_error("invalid prolog end");

                continue;
            }

            // <! ... >
            if (skip('!'))
                return toolkit::make_error("not yet implemented");

            skip_start = true;
            break;
        }

        break;
    }

    node document;
    if (auto res = parse_element(skip_start) >> document; !res)
        return res;

    return document;
}

toolkit::result<xml::node> xml::parser::parse_element(bool skip_start)
{
    // <{tag} [{attribute}...] />
    // <{tag} [{attribute}...]>...</{tag}>

    if (!skip_start && !skip('<'))
        return toolkit::make_error("invalid element start");

    std::string tag;
    while (!(is_whitespace(m_Buffer) || at('>') || at('/')))
        tag += pop();

    std::unordered_map<std::string, node> attributes;
    while (!(at('>') || at('/')))
    {
        if (skip_whitespace())
            continue;

        std::string name;
        while (!(is_whitespace(m_Buffer) || at('=') || at('>') || at('/')))
            name += pop();

        if (auto res = parse_attribute() >> attributes[name]; !res)
            return res;
    }

    std::vector<node> elements;
    if (skip('>'))
    {
        while (true)
        {
            if (skip_whitespace())
                continue;

            if (skip('<'))
            {
                if (skip('/'))
                {
                    if (!skip(tag))
                        return toolkit::make_error("invalid element closing tag");

                    if (!skip('>'))
                        return toolkit::make_error("invalid element closing tag");

                    break;
                }

                if (skip('!'))
                {
                    if (at('-'))
                        if (auto res = skip_comment(true); !res)
                            return res;

                    if (at('['))
                        if (auto res = parse_cdata(true); !res)
                            return res;

                    continue;
                }

                node data;
                if (auto res = parse_element(true) >> data; !res)
                    return res;

                elements.push_back(std::move(data));
            }
            else
            {
                std::string data;
                if (auto res = parse_text_no_skip('<') >> data; !res)
                    return res;

                elements.emplace_back(std::move(data));
            }
        }
    }
    else if (!skip("/>"))
        return toolkit::make_error("invalid element end");

    return {
        element
        {
            { "tag", tag },
            { "attributes", attributes },
            { "elements", elements },
        }
    };
}

toolkit::result<xml::node> xml::parser::parse_attribute()
{
    // {<nothing>} -> true
    // ="{value}"
    // ='{value}'

    if (!skip('='))
        return { true };

    std::string_view end;
    if (skip('"'))
        end = "\"";
    else if (skip('\''))
        end = "'";
    else
        return toolkit::make_error("invalid attribute start");

    std::string data;
    if (auto res = parse_text(end) >> data; !res)
        return res;

    return { std::move(data) };
}

toolkit::result<> xml::parser::skip_comment(const bool skip_start)
{
    // <!-- ... -->

    if (!skip(skip_start ? "--" : "<!--"))
        return toolkit::make_error("invalid comment start");

    std::string data;
    if (auto res = parse_text("-->") >> data; !res)
        return res;

    return {};
}

toolkit::result<xml::node> xml::parser::parse_cdata(const bool skip_start)
{
    // <![CDATA[ ... ]]>

    if (!skip(skip_start ? "[CDATA[" : "<![CDATA["))
        return toolkit::make_error("invalid cdata start");

    std::string data;
    if (auto res = parse_text("]]>") >> data; !res)
        return res;

    return { std::move(data) };
}

toolkit::result<std::string> xml::parser::parse_text(const std::string_view end)
{
    std::string value;

    for (std::size_t end_pos{}; end_pos < end.size();)
    {
        if (skip(end[end_pos]))
        {
            ++end_pos;
            continue;
        }

        if (end_pos)
        {
            value += end.substr(0, end_pos);
            end_pos = {};
        }

        value += pop();
    }

    return value;
}

toolkit::result<xml::string> xml::parser::parse_text_no_skip(const char end)
{
    std::string value;

    while (true)
    {
        if (at(end))
            break;

        value += pop();
    }

    return value;
}

void xml::parser::get()
{
    m_Buffer = m_Stream.get();
}

char xml::parser::pop()
{
    const auto c = m_Buffer;
    get();
    return static_cast<char>(c);
}

bool xml::parser::at(const char c) const
{
    return m_Buffer == c;
}

bool xml::parser::skip(const char c)
{
    if (m_Buffer == c)
    {
        m_Buffer = m_Stream.get();
        return true;
    }
    return false;
}

bool xml::parser::skip(const std::string_view s)
{
    for (const auto c : s)
        if (!skip(c))
            return false;
    return true;
}

bool xml::parser::skip_whitespace()
{
    if (!is_whitespace(m_Buffer))
        return false;

    do
        get();
    while (is_whitespace(m_Buffer));

    return true;
}
