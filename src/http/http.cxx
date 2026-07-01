#include <http/http.hxx>

#include <toolkit/string.hxx>

#include <istream>

toolkit::result<> http::ParseStatus(
    std::istream &stream,
    HttpStatusCode &status_code,
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

void http::ParseHeaders(std::istream &stream, HttpHeaders &headers)
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
