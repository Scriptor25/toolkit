#pragma once

#include <data/result.hxx>
#include <toml/toml.hxx>

#include <format>

namespace toml
{
    class Parser final
    {
        using Key = std::vector<data::Key>;

    public:
        explicit Parser(std::istream &stream);

        data::result<Node> Parse();

    protected:
        data::result<Node> ParseValue();

        data::result<Node> ParseNumber();
        data::result<Node> ParseString();
        data::result<Node> ParseLocalDate();
        data::result<Node> ParseLocalTime();
        data::result<Node> ParseDateTime();
        data::result<Node> ParseArray();
        data::result<Node> ParseTable();

        data::result<Key> ParseKey();

        static data::result<Node *> MakeNodeKey(Node &node, const Key &key);
        static data::result<Node *> MakeNodeKey(Table &nodes, const Key &key);

        void Get();
        char Pop();

        [[nodiscard]] char Peek() const;

        data::result<uint8_t> PopHalfByte();
        data::result<uint8_t> PopByte();

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
