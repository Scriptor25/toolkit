#pragma once

#include <json/concepts.hxx>
#include <json/forward.hxx>
#include <json/json.hxx>

namespace json
{
    template<node N>
    bool from_json(N &&node, Node &value)
    {
        value = std::forward<N>(node);
        return true;
    }

    template<node T>
    void to_json(Node &node, T &&value)
    {
        node = std::forward<T>(value);
    }

    template<node N, primitive T>
    bool from_json(N &&node, T &value)
    {
        if (node.template Is<T>())
        {
            value = node.template Get<T>();
            return true;
        }

        return false;
    }

    template<primitive T>
    void to_json(Node &node, T &&value)
    {
        node = Node(std::forward<T>(value));
    }

    template<node N, floating_point T>
    bool from_json(N &&node, T &value)
    {
        if (Number val; from_json(std::forward<N>(node), val))
        {
            value = static_cast<T>(val);
            return true;
        }

        return false;
    }

    template<floating_point T>
    void to_json(Node &node, T &&value)
    {
        to_json(node, static_cast<Number>(std::forward<T>(value)));
    }

    template<node N, integral T>
    bool from_json(N &&node, T &value)
    {
        if (Number val; from_json(std::forward<N>(node), val))
        {
            value = static_cast<T>(val);
            return true;
        }

        return false;
    }

    template<integral T>
    void to_json(Node &node, T &&value)
    {
        to_json(node, static_cast<Number>(std::forward<T>(value)));
    }

    template<node N, typename T>
    bool from_json(N &&node, std::vector<T> &value)
    {
        if (!node.template Is<Array>())
            return false;

        value.resize(node.size());

        auto ok = true;
        for (std::size_t i = 0; i < node.size(); ++i)
            ok &= from_json(node[i], value[i]);

        return ok;
    }

    template<vector T>
    void to_json(Node &node, T &&value)
    {
        node = Array(value.size());

        for (std::size_t i = 0; i < value.size(); ++i)
            to_json(node[i], value[i]);
    }

    template<node N, typename T, std::size_t S>
    bool from_json(N &&node, std::array<T, S> &value)
    {
        if (!node.template Is<Array>() || node.size() != S)
            return false;

        value.resize(S);

        auto ok = true;
        for (std::size_t i = 0; i < S; ++i)
            ok &= from_json(node[i], value[i]);

        return ok;
    }

    template<array T>
    void to_json(Node &node, T &&value)
    {
        node = Array(value.size());

        for (std::size_t i = 0; i < value.size(); ++i)
            to_json(node[i], value[i]);
    }

    template<node N, typename T>
    bool from_json(N &&node, std::set<T> &value)
    {
        if (std::vector<T> val; from_json(std::forward<N>(node), val))
        {
            value = { std::make_move_iterator(val.begin()), std::make_move_iterator(val.end()) };
            return true;
        }

        return false;
    }

    template<set T>
    void to_json(Node &node, T &&value)
    {
        to_json(node, std::vector(value.begin(), value.end()));
    }

    template<node N, typename T>
    bool from_json(N &&node, std::map<Key, T> &value)
    {
        if (!node.template Is<Object>())
            return false;

        auto ok = true;
        for (auto &&[key, val] : std::forward<N>(node))
            ok &= from_json(val, value[key]);

        return ok;
    }

    template<map T>
    void to_json(Node &node, T &&value)
    {
        node = Object();

        for (auto &&[key, val] : std::forward<T>(value))
            to_json(node[key], val);
    }

    template<node N, typename T>
    bool from_json(N &&node, std::optional<T> &value)
    {
        if (node.template Is<Undefined>() || node.template Is<Null>())
        {
            value = std::nullopt;
            return true;
        }

        if (T val; from_json(std::forward<N>(node), val))
        {
            value = std::move(val);
            return true;
        }

        return false;
    }

    template<optional T>
    void to_json(Node &node, T &&value)
    {
        if (value.has_value())
        {
            to_json(node, value.value());
            return;
        }

        node = Node();
    }

    template<node N, typename... T>
    bool from_json(N &&node, std::variant<T...> &value)
    {
        return ([&]() -> bool
        {
            if (T val; from_json(node, val))
            {
                value = std::move(val);
                return true;
            }
            return false;
        }() || ...);
    }

    template<variant T>
    void to_json(Node &node, T &&value)
    {
        std::visit(
            [&node]<typename V>(V &&val)
            {
                to_json(node, val);
            },
            std::forward<T>(value));
    }

    template<node N, typename T>
    bool from_json_opt(N &&node, T &value, T default_value)
    {
        if (std::optional<T> val; from_json(std::forward<N>(node), val))
        {
            value = val.value_or(std::move(default_value));
            return true;
        }

        return false;
    }
}
