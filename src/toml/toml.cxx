#include <toml/parser.hxx>

#include <iostream>

std::istream &data::node_traits_t<
    toml::boolean_t,
    toml::integer_t,
    toml::floating_point_t,
    toml::string_t,
    toml::local_date_t,
    toml::local_time_t,
    toml::date_time_t
>::parse(std::istream &stream, toml::node_t &node)
{
    toml::parser parser(stream);
    if (auto res = parser.parse())
        node = *std::move(res);
    else
        std::cerr << res.error() << std::endl;
    return stream;
}
