#pragma once

#include <json/json.hxx>

#include <toolkit/result.hxx>

#include <cstdint>

namespace json
{
    class parser
    {
    public:
        explicit parser(std::istream &stream);

        [[nodiscard]] toolkit::result<node_t> parse();

    protected:
        [[nodiscard]] toolkit::result<node_t> parse_number();
        [[nodiscard]] toolkit::result<node_t> parse_string();
        [[nodiscard]] toolkit::result<node_t> parse_array();
        [[nodiscard]] toolkit::result<node_t> parse_object();

        void get();
        char pop();

        [[nodiscard]] toolkit::result<uint8_t> pop_nibble();
        [[nodiscard]] toolkit::result<uint8_t> pop_byte();

        [[nodiscard]] bool at(char c) const;

        bool skip(char c);
        bool skip(std::string_view s);
        bool skip_whitespace();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
