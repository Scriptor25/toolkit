#include <toml/parser.hxx>

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
    if (auto result = parser.Parse())
    {
        node = *std::move(result);
    }
    else
    {
        std::cerr << result.error() << std::endl;
    }
    return stream;
}
