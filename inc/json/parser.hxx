#pragma once

#include <data/result.hxx>
#include <json/json.hxx>

#include <format>

namespace json
{
    class Parser final
    {
    public:
        explicit Parser(std::istream &stream);

        data::result<Node> Parse();

    protected:
        data::result<Node> ParseNumber();
        data::result<Node> ParseString();
        data::result<Node> ParseArray();
        data::result<Node> ParseObject();

        void Get();
        char Pop();

        data::result<uint8_t> PopHalfByte();
        data::result<uint8_t> PopByte();

        [[nodiscard]] bool At(char c) const;

        bool Skip(char c);
        bool Skip(std::string_view s);
        bool SkipWhitespace();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
