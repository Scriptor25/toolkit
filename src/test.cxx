#include <data/json_toml.hxx>
#include <json/json.hxx>
#include <toml/toml.hxx>

struct test_t
{
    toml::integer_t foo{};
    toml::boolean_t bar{};
};

template<>
struct data::serializer<test_t>
{
    static bool from_data(const toml::node_t &node, test_t &value)
    {
        if (!node.is<toml::table_t>())
            return false;

        auto ok = true;

        ok &= node["foo"] >> value.foo;
        ok &= node["bar"] >> value.bar;

        return ok;
    }
};

int main()
{
    json::node_t json_node = json::object_t
    {
        { "foo", 123 },
        { "bar", true },
    };

    toml::node_t toml_node = json_node;

    test_t value;
    toml_node >> value;
}
