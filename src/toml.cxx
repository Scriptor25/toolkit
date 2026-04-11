#include <toml/parser.hxx>
#include <toml/toml.hxx>

#include <iostream>

std::istream &data::NodeTraits<
    toml::Boolean,
    toml::Integer,
    toml::FloatingPoint,
    toml::String,
    toml::LocalDate,
    toml::LocalTime,
    toml::DateTime
>::parse(std::istream &stream, toml::Node &node)
{
    toml::Parser parser(stream);
    if (auto exp = parser.Parse())
        node = *exp;
    else
        std::cerr << exp.error() << std::endl;
    return stream;
}
