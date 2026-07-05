#include <http/http.hxx>

#include <toolkit/string.hxx>

#include <istream>

toolkit::result<> http::parse_status(
    std::istream &stream,
    status_code_t &status_code,
    std::string &status_message)
{
    std::string http_version;
    stream >> http_version;

    if (http_version != "HTTP/1.1")
    {
        return toolkit::make_error("invalid http version '{}'.", http_version);
    }

    stream >> status_code;
    toolkit::get_line(stream, status_message, EOL);

    status_message = toolkit::trim(std::move(status_message));
    return {};
}

void http::parse_headers(std::istream &stream, headers_t &headers)
{
    headers.clear();

    std::string line;
    while (toolkit::get_line(stream, line, EOL))
    {
        if (line.empty())
        {
            break;
        }

        const auto colon = line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }

        auto key = line.substr(0, colon);
        auto val = line.substr(colon + 1);

        key = toolkit::trim(std::move(key));
        val = toolkit::trim(std::move(val));

        headers.emplace(toolkit::lowercase(std::move(key)), std::move(val));
    }
}

std::ostream &operator<<(std::ostream &stream, const http::method_t method)
{
    static const std::map<http::method_t, const char *> map
    {
        { http::method_t::GET, "GET" },
        { http::method_t::HEAD, "HEAD" },
        { http::method_t::POST, "POST" },
        { http::method_t::PUT, "PUT" },
        { http::method_t::DELETE, "DELETE" },
        { http::method_t::CONNECT, "CONNECT" },
        { http::method_t::OPTIONS, "OPTIONS" },
        { http::method_t::TRACE, "TRACE" },
    };

    if (const auto it = map.find(method); it != map.end())
    {
        return stream << it->second;
    }

    return stream << "undefined";
}

std::ostream &operator<<(std::ostream &stream, http::status_code_t status_code)
{
    return stream << static_cast<int>(status_code);
}

std::istream &operator>>(std::istream &stream, http::status_code_t &status_code)
{
    int status_code_int;
    stream >> status_code_int;
    status_code = static_cast<http::status_code_t>(status_code_int);
    return stream;
}
