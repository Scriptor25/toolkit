#pragma once

#include <toml/toml.hxx>

#include <toolkit/result.hxx>

#include <cstdint>

namespace toml
{
    class parser
    {
        using key_t = std::vector<std::string>;

    public:
        explicit parser(std::istream &stream);

        toolkit::result<node> parse();

    protected:
        toolkit::result<node> parse_value();

        toolkit::result<node> parse_number();
        toolkit::result<node> parse_string();
        toolkit::result<node> parse_local_date();
        toolkit::result<node> parse_local_time();
        toolkit::result<node> parse_date_time();
        toolkit::result<node> parse_array();
        toolkit::result<node> parse_table();

        toolkit::result<key_t> parse_key();

        static toolkit::result<node *> find_node(node &root, const key_t &key);
        static toolkit::result<node *> find_node(table &root, const key_t &key);

        void get();
        char pop();

        [[nodiscard]] char peek() const;

        toolkit::result<uint8_t> pop_nibble();
        toolkit::result<uint8_t> pop_byte();

        [[nodiscard]] bool at(int c) const;
        [[nodiscard]] bool at_key() const;
        [[nodiscard]] bool at_digit(int base) const;

        bool skip(char c);
        bool skip(std::string_view s);
        bool skip_whitespace();
        bool skip_comment();
        bool skip_eol();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
