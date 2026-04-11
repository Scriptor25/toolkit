#include <toml/parser.hxx>
#include <toml/toml.hxx>

#include <iostream>

std::istream &operator>>(std::istream &stream, toml::Node &node)
{
    toml::Parser parser(stream);
    if (auto exp = parser.Parse())
        node = *exp;
    else
        std::cerr << exp.error() << std::endl;
    return stream;
}
