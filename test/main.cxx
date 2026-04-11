#include <json/json.hxx>
#include <toml/toml.hxx>

#include <cross/json_toml.hxx>

#include <fstream>
#include <iomanip>
#include <iostream>

int main()
{
    std::ifstream stream("test.toml");

    toml::Node toml;
    stream >> toml;

    json::Node json = toml;

    std::cout << std::setw(2) << json << std::endl;
}
