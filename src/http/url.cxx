#include <http/url.hxx>

#include <iostream>

void http::ParseURL(URL &dst, std::string_view src)
{
    const auto scheme_end = src.find("://");
    const auto scheme = src.substr(0, scheme_end);

    dst.Scheme = scheme;

    const auto host_begin = scheme_end + 3;
    const auto path_begin = src.find('/', host_begin);

    auto host_port = path_begin == std::string_view::npos
                         ? src.substr(host_begin)
                         : src.substr(host_begin, path_begin - host_begin);

    dst.Pathname = path_begin == std::string_view::npos
                       ? "/"
                       : src.substr(path_begin);

    if (const auto colon = host_port.find(':'); colon != std::string_view::npos)
    {
        const std::string port(host_port.substr(colon + 1));

        dst.Host = host_port.substr(0, colon);
        dst.Port = static_cast<uint16_t>(std::stoi(port));
    }
    else
    {
        dst.Host = host_port;
        dst.Port = scheme == "https" ? 443 : scheme == "http" ? 80 : 0;
    }
}

http::URL http::ParseURL(const std::string_view src)
{
    URL dst;
    ParseURL(dst, src);
    return dst;
}

std::ostream &operator<<(std::ostream &stream, const http::URL &location)
{
    return stream
           << location.Scheme
           << "://"
           << location.Host
           << ":"
           << location.Port
           << location.Pathname;
}
