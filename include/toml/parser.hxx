#pragma once

#include <toolkit/result.hxx>

#include <toml/toml.hxx>

#include <cstdint>

namespace toml
{
    class Parser final
    {
        using Key = std::vector<data::Key>;

    public:
        explicit Parser(std::istream &stream);

        toolkit::result<Node> Parse();

    protected:
        toolkit::result<Node> ParseValue();

        toolkit::result<Node> ParseNumber();
        toolkit::result<Node> ParseString();
        toolkit::result<Node> ParseLocalDate();
        toolkit::result<Node> ParseLocalTime();
        toolkit::result<Node> ParseDateTime();
        toolkit::result<Node> ParseArray();
        toolkit::result<Node> ParseTable();

        toolkit::result<Key> ParseKey();

        static toolkit::result<Node *> MakeNodeKey(Node &node, const Key &key);
        static toolkit::result<Node *> MakeNodeKey(Table &nodes, const Key &key);

        void Get();
        char Pop();

        [[nodiscard]] char Peek() const;

        toolkit::result<uint8_t> PopHalfByte();
        toolkit::result<uint8_t> PopByte();

        [[nodiscard]] bool At(int c) const;
        [[nodiscard]] bool AtKey() const;
        [[nodiscard]] bool AtDigit(int base) const;

        bool Skip(char c);
        bool Skip(std::string_view s);
        bool SkipWhitespace();
        bool SkipComment();
        bool SkipEoL();

    private:
        std::istream &m_Stream;
        int m_Buffer;
    };
}
