#pragma once

#include <toml/toml.hxx>

#include <expected>
#include <format>

namespace toml
{
    template<typename T>
    using Exp = std::expected<T, std::string>;

    template<typename... Args>
    auto Error(std::format_string<Args...> format, Args &&... args)
    {
        return std::unexpected{ std::format(std::move(format), std::forward<Args>(args)...) };
    }

    class Parser final
    {
        using Key = std::vector<Node::Key>;

    public:
        explicit Parser(std::istream &stream);

        Exp<Node> Parse();

    protected:
        Exp<Node> ParseValue();

        Exp<Node> ParseNumber();
        Exp<Node> ParseString();
        Exp<Node> ParseLocalDate();
        Exp<Node> ParseLocalTime();
        Exp<Node> ParseDateTime();
        Exp<Node> ParseArray();
        Exp<Node> ParseTable();

        Exp<Key> ParseKey();

        static Exp<Node *> MakeNodeKey(Node &node, const Key &key);
        static Exp<Node *> MakeNodeKey(Node::Map &nodes, const Key &key);

        void Get();
        char Pop();

        [[nodiscard]] char Peek() const;

        Exp<unsigned char> PopHalfByte();
        Exp<unsigned char> PopByte();

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
