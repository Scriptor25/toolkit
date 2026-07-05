#include <http/url.hxx>

#include <iostream>

void http::url::parse(url &dst, std::string_view src)
{
    const auto scheme_end = src.find("://");
    const auto scheme = src.substr(0, scheme_end);

    dst.scheme = scheme;

    const auto host_begin = scheme_end + 3;
    const auto path_begin = src.find('/', host_begin);

    auto host_port = path_begin == std::string_view::npos
                         ? src.substr(host_begin)
                         : src.substr(host_begin, path_begin - host_begin);

    dst.pathname = path_begin == std::string_view::npos
                       ? "/"
                       : src.substr(path_begin);

    if (const auto colon = host_port.find(':'); colon != std::string_view::npos)
    {
        const std::string port(host_port.substr(colon + 1));

        dst.host = host_port.substr(0, colon);
        dst.port = static_cast<uint16_t>(std::stoi(port));
    }
    else
    {
        dst.host = host_port;
        dst.port = scheme == "https" ? 443 : scheme == "http" ? 80 : 0;
    }
}

http::url http::url::parse(std::string_view src)
{
    url dst;
    parse(dst, src);
    return dst;
}

std::ostream &operator<<(std::ostream &stream, const http::url &location)
{
    return stream
           << location.scheme
           << "://"
           << location.host
           << ":"
           << location.port
           << location.pathname;
}
