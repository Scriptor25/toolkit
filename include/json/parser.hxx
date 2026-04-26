#pragma once

#include <toolkit/result.hxx>

#include <json/json.hxx>

#include <cstdint>

namespace json
{
    class Parser final
    {
    public:
        explicit Parser(std::istream &stream);

        toolkit::result<Node> Parse();

    protected:
        toolkit::result<Node> ParseNumber();
        toolkit::result<Node> ParseString();
        toolkit::result<Node> ParseArray();
        toolkit::result<Node> ParseObject();

        void Get();
        char Pop();

        toolkit::result<uint8_t> PopHalfByte();
        toolkit::result<uint8_t> PopByte();

        [[nodiscard]] bool At(char c) const;

        bool Skip(char c);
        bool Skip(std::string_view s);
        bool SkipWhitespace();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
