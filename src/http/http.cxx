#include <http/http.hxx>

#include <toolkit/string.hxx>

#include <istream>

toolkit::result<> http::parse_status(
    std::istream &stream,
    status_code &code,
    std::string &message)
{
    std::string http_version;
    stream >> http_version;

    if (http_version != "HTTP/1.1")
    {
        return toolkit::make_error("invalid http version '{}'.", http_version);
    }

    stream >> code;
    toolkit::get_line(stream, message, EOL);

    message = toolkit::trim(message);
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

        key = toolkit::trim(key);
        val = toolkit::trim(val);

        headers.emplace(toolkit::lowercase(key), std::move(val));
    }
}

std::ostream &operator<<(std::ostream &stream, const http::method method)
{
    static const std::map<http::method, const char *> map
    {
        { http::method::get, "GET" },
        { http::method::head, "HEAD" },
        { http::method::post_, "POST" },
        { http::method::put, "PUT" },
        { http::method::delete_, "DELETE" },
        { http::method::connect, "CONNECT" },
        { http::method::options, "OPTIONS" },
        { http::method::trace, "TRACE" },
    };

    if (const auto it = map.find(method); it != map.end())
    {
        return stream << it->second;
    }

    return stream << "undefined";
}

std::ostream &operator<<(std::ostream &stream, http::status_code code)
{
    return stream << static_cast<int>(code);
}

std::istream &operator>>(std::istream &stream, http::status_code &code)
{
    int status_code_int;
    stream >> status_code_int;
    code = static_cast<http::status_code>(status_code_int);
    return stream;
}
