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

        toolkit::result<node_t> parse();

    protected:
        toolkit::result<node_t> parse_value();

        toolkit::result<node_t> parse_number();
        toolkit::result<node_t> parse_string();
        toolkit::result<node_t> parse_local_date();
        toolkit::result<node_t> parse_local_time();
        toolkit::result<node_t> parse_date_time();
        toolkit::result<node_t> parse_array();
        toolkit::result<node_t> parse_table();

        toolkit::result<key_t> parse_key();

        static toolkit::result<node_t *> find_node(node_t &node, const key_t &key);
        static toolkit::result<node_t *> find_node(table_t &nodes, const key_t &key);

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
