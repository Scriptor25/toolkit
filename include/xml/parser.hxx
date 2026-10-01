#pragma once

#include <xml/xml.hxx>

#include <toolkit/result.hxx>

namespace xml
{
    class parser
    {
    public:
        explicit parser(std::istream &stream);

        [[nodiscard]] toolkit::result<node> parse();

    protected:
        [[nodiscard]] toolkit::result<node> parse_document();
        [[nodiscard]] toolkit::result<node> parse_element(bool skip_start);
        [[nodiscard]] toolkit::result<attribute> parse_attribute();

        [[nodiscard]] toolkit::result<> skip_comment(bool skip_start);

        [[nodiscard]] toolkit::result<node> parse_cdata(bool skip_start);
        [[nodiscard]] toolkit::result<string> parse_text(std::string_view end);
        [[nodiscard]] toolkit::result<string> parse_text_no_skip(char end);

        void get();
        char pop();

        [[nodiscard]] bool at(char c) const;

        bool skip(char c);
        bool skip(std::string_view s);
        bool skip_whitespace();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
