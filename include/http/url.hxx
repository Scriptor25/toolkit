#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace http
{
    struct URL
    {
        std::string Scheme;
        std::string Host;
        uint16_t Port{};
        std::string Pathname;
    };

    void ParseURL(URL &dst, std::string_view src);
    URL ParseURL(std::string_view src);
}

std::ostream &operator<<(std::ostream &stream, const http::URL &location);
