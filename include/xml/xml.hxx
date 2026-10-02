#pragma once

#include <charconv>
#include <data/node.hxx>

#include <string>

namespace xml
{
    using undefined = data::undefined_type;
    using boolean = bool;
    using string = std::string;

    struct element;

    using node = data::node_base<
        string,
        element
    >;

    struct attribute
    {
        using value_type = std::variant<undefined, boolean, string>;

        explicit attribute();

        attribute(const attribute &other) = default;
        attribute &operator=(const attribute &other) = default;

        attribute(attribute &&other) noexcept = default;
        attribute &operator=(attribute &&other) noexcept = default;

        explicit attribute(const value_type &value);

        attribute &operator=(const value_type &value);

        explicit attribute(value_type &&value);

        attribute &operator=(value_type &&value);

        template<data::primitive<attribute> T>
        attribute(T &&value)
            : val(std::forward<T>(value))
        {
        }

        template<data::primitive<attribute> T>
        attribute &operator=(T &&value)
        {
            val = std::forward<T>(value);
            return *this;
        }

        template<data::assignable<attribute, value_type> T>
        attribute(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
        }

        template<data::assignable<attribute, value_type> T>
        attribute &operator=(T &&value)
        {
            to_data_fn(*this, std::forward<T>(value));
            return *this;
        }

        template<typename T>
        bool operator>>(T &value) const
        {
            return data::from_data_fn(*this, value);
        }

        auto &&operator*() &&
        {
            return val;
        }

        auto &operator*() &
        {
            return val;
        }

        const auto &operator*() const &
        {
            return val;
        }

        template<data::primitive<attribute> T>
        [[nodiscard]] auto is() const
        {
            return std::holds_alternative<T>(val);
        }

        template<data::primitive<attribute> T>
        auto &&get()
        {
            return std::get<T>(val);
        }

        template<data::primitive<attribute> T>
        auto &&get() const
        {
            return std::get<T>(val);
        }

        value_type val;
    };

    struct element
    {
        template<typename T>
        bool operator>>(T &value) const
        {
            return data::from_data_fn(*this, value);
        }

        std::string get_text() const;

        const element *find(const std::string &key) const;
        [[nodiscard]] std::vector<const element *> find_all(const std::string &key) const;

        std::string tag;
        std::unordered_map<std::string, attribute> attributes;
        std::vector<node> nodes;

        std::vector<const element *> elements;
        std::unordered_map<std::string, std::vector<const element *>> elements_map;
    };

    using vec = node::vec_type;
    using map = node::map_type;
}

template<>
struct data::node_traits<
            xml::string,
            xml::element
        >
{
    static std::ostream &print(std::ostream &stream, const xml::node &node);
    static std::istream &parse(std::istream &stream, xml::node &node);
};

namespace data
{
    template<floating_point<xml::node> T>
    struct serializer<xml::node, T>
    {
        static bool from_data(const xml::node &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(val.data(), val.data() + val.size(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::node &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };

    template<integral<xml::node> T>
    struct serializer<xml::node, T>
    {
        static bool from_data(const xml::node &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(val.data(), val.data() + val.size(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::node &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };

    template<primitive<xml::attribute> T>
    struct serializer<xml::attribute, T>
    {
        static bool from_data(const xml::attribute &node, T &value)
        {
            if (node.is<T>())
            {
                value = node.get<T>();
                return true;
            }

            return false;
        }

        template<toolkit::same_as<T> U>
        static void to_data(xml::attribute &node, U &&value)
        {
            node = N(std::forward<U>(value));
        }
    };

    template<floating_point<xml::node> T>
    struct serializer<xml::attribute, T>
    {
        static bool from_data(const xml::attribute &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(val.data(), val.data() + val.size(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::attribute &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };

    template<integral<xml::attribute> T>
    struct serializer<xml::attribute, T>
    {
        static bool from_data(const xml::attribute &node, T &value)
        {
            if (xml::string val; node >> val)
            {
                const auto result = std::from_chars(val.data(), val.data() + val.size(), value);
                return result.ec == std::errc{};
            }

            return false;
        }

        static void to_data(xml::attribute &node, T &&value)
        {
            node = std::to_string(std::forward<T>(value));
        }
    };
}
