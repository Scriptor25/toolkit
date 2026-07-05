#include <toml/parser.hxx>

#include <iostream>

std::istream &data::node_traits<
    toml::boolean,
    toml::integer,
    toml::floating_point,
    toml::string,
    toml::local_date,
    toml::local_time,
    toml::date_time
>::parse(std::istream &stream, toml::node &node)
{
    toml::parser parser(stream);
    if (auto res = parser.parse())
        node = *std::move(res);
    else
        std::cerr << res.error() << std::endl;
    return stream;
}
