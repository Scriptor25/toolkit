#pragma once

#include <json/concepts.hxx>
#include <json/forward.hxx>

namespace json
{
    template<node N, typename T>
    bool from_json(N &&node, T &value) = delete;

    template<typename T>
    void to_json(Node &node, T &&value) = delete;

    template<node N, typename T>
    bool operator>>(N &&node, T &value)
    {
        return from_json(std::forward<N>(node), value);
    }

    struct Node final
    {
        template<typename T, typename V, typename M>
        struct iterator_base final
        {
            using value_type = std::pair<Key, T &>;

            iterator_base() = default;

            explicit iterator_base(V &&it)
                : it(std::forward<V>(it))
            {
            }

            explicit iterator_base(M &&it)
                : it(std::forward<M>(it))
            {
            }

            auto operator!=(iterator_base other) const
            {
                return it != other.it;
            }

            auto operator*() const
            {
                struct
                {
                    auto operator()(V &&i) -> value_type
                    {
                        return { {}, *i };
                    }

                    auto operator()(M &&i) -> value_type
                    {
                        return { i->first, i->second };
                    }
                } visitor;

                return std::visit(visitor, it);
            }

            auto &&operator++()
            {
                std::visit(
                    [](auto &it)
                    {
                        ++it;
                    },
                    it);

                return *this;
            }

            std::variant<std::monostate, V, M> it{};
        };

        using iterator = iterator_base<
            Node,
            Array::iterator,
            Object::iterator>;
        using const_iterator = iterator_base<
            const Node,
            Array::const_iterator,
            Object::const_iterator>;

        Node() = default;

        Node(const Node &node) = default;
        Node &operator=(const Node &node) = default;

        Node(Node &&node) noexcept = default;
        Node &operator=(Node &&node) noexcept = default;

        explicit Node(const NodeValue &value);
        Node &operator=(const NodeValue &value);

        explicit Node(NodeValue &&value);
        Node &operator=(NodeValue &&value);

        template<primitive T>
        Node(T &&value)
            : Value(std::forward<T>(value))
        {
        }

        template<primitive T>
        auto &&operator=(T &&value)
        {
            Value = std::forward<T>(value);
            return *this;
        }

        template<assignable T>
        Node(T &&value)
        {
            to_json(*this, std::forward<T>(value));
        }

        template<assignable T>
        auto &&operator=(T &&value)
        {
            to_json(*this, std::forward<T>(value));
            return *this;
        }

        template<primitive T>
        auto Is() const
        {
            return std::holds_alternative<T>(Value);
        }

        template<primitive T>
        auto &&Get()
        {
            return std::get<T>(Value);
        }

        template<primitive T>
        auto &&Get() const
        {
            return std::get<T>(Value);
        }

        bool operator!() const;

        std::ostream &Print(std::ostream &stream) const;

        iterator begin();
        iterator end();

        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;

        [[nodiscard]] Index size() const;

        Node &operator[](Index index);
        Node operator[](Index index) const;

        Node &operator[](const Key &key);
        Node operator[](const Key &key) const;

        NodeValue Value{};
    };

    std::ostream &operator<<(std::ostream &stream, const Node &node);
    std::istream &operator>>(std::istream &stream, Node &node);
}
