#include <data/json_toml.hxx>
#include <json/json.hxx>
#include <toml/toml.hxx>
#include <toolkit/result.hxx>

struct test_t
{
    toml::integer foo{};
    toml::boolean bar{};
};

template<>
struct data::serializer<test_t>
{
    static bool from_data(const toml::node &node, test_t &value)
    {
        if (!node.is<toml::table>())
            return false;

        auto ok = true;

        ok &= node["foo"] >> value.foo;
        ok &= node["bar"] >> value.bar;

        return ok;
    }
};

int main()
{
    json::node json_node = json::object
    {
        { "foo", 123 },
        { "bar", true },
    };

    toml::node toml_node = json_node;

    test_t value;
    toml_node >> value;

    toolkit::result<test_t> x;
    toml::integer y;
    x.extract(&test_t::foo) >> y;
}
