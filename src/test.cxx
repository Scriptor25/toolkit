#include <data/json_toml.hxx>
#include <json/json.hxx>
#include <toml/toml.hxx>
#include <xml/xml.hxx>

#include <toolkit/result.hxx>

#include <sstream>

struct test_t
{
    toml::integer foo{};
    toml::boolean bar{};
};

template<>
struct data::serializer<toml::node, test_t>
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

template<>
struct data::serializer<xml::node, test_t>
{
    static bool from_data(const xml::node &node, test_t &value)
    {
        if (!node.is<xml::element>())
            return false;

        const auto &tag = node["tag"];
        const auto &attributes = node["attributes"];

        if (!tag || !attributes)
            return false;

        if (!tag.is<xml::string>())
            return false;

        if (!attributes.is<xml::element>())
            return false;

        if (std::string tag_str; tag >> tag_str, tag_str != "test")
            return false;

        auto ok = true;

        ok &= attributes["foo"] >> value.foo;
        ok &= attributes["bar"] >> value.bar;

        return ok;
    }
};

int main()
{
    const auto *xml = R"(<test foo="123" bar />)";
    std::istringstream xml_stream(xml);

    xml::node xml_node;
    xml_stream >> xml_node;

    test_t xml_value;
    xml_node >> xml_value;

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
