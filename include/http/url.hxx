#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace http
{
    struct url
    {
        static void parse(url &dst, std::string_view src);
        static url parse(std::string_view src);

        std::string scheme;
        std::string host;
        uint16_t port{};
        std::string pathname;
    };
}

std::ostream &operator<<(std::ostream &stream, const http::url &location);
