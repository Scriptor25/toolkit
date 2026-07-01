#include <http/client.hxx>
#include <http/http.hxx>
#include <http/url.hxx>

#include <toolkit/defer.hxx>
#include <toolkit/string.hxx>

#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>

http::Client::Client(Transport &transport)
    : m_Transport(transport)
{
}

static void set_header_if_missing(http::HttpHeaders &headers, const std::string &key, const std::string &val)
{
    if (headers.contains(key) || headers.contains(toolkit::lowercase(key)))
        return;

    headers.emplace(key, val);
}

toolkit::result<> http::Client::Fetch(HttpRequest request, HttpResponse &response)
{
    int fd;
    if (auto res = m_Transport.open(request.Location) >> fd; !res)
        return res;

    auto guard0 = toolkit::defer(
        [this](auto x)
        {
            m_Transport.close(x);
        },
        fd);

    set_header_if_missing(request.Headers, "Host", request.Location.Host);
    set_header_if_missing(request.Headers, "Connection", "close");
    set_header_if_missing(request.Headers, "Accept-Encoding", "identity");
    set_header_if_missing(request.Headers, "User-Agent", "unvm/0.1");

    std::stringstream packet;
    packet << request.Method << ' ' << request.Location.Pathname << ' ' << "HTTP/1.1" << EOL;
    for (auto &[key, val] : request.Headers)
        packet << key << ": " << val << EOL;
    packet << EOL;

    if (Write(fd, packet.str()) < 0)
        return toolkit::make_error("failed to send header.");

    char chunk[4096];

    if (request.Body)
    {
        size_t count = 0;

        while (true)
        {
            request.Body->read(chunk, sizeof(chunk));
            const size_t len = request.Body->gcount();

            if (len <= 0)
                break;

            if (Write(fd, { chunk, len }) < 0)
                return toolkit::make_error("failed to send chunk.");

            count += len;
        }
    }

    std::string header_block;
    if (auto res = ReadUntil(fd, header_block, EOL2); !res)
        return toolkit::make_error("failed to read header block: {}", res.error());

    auto headers_end = header_block.find(EOL2);
    auto headers = header_block.substr(0, headers_end);
    auto body_prefetch = header_block.substr(headers_end + 4);

    std::istringstream headers_stream(headers);

    std::string status_line;
    toolkit::get_line(headers_stream, status_line, EOL);

    std::istringstream status_stream(status_line);
    if (auto res = ParseStatus(status_stream, response.StatusCode, response.StatusMessage); !res)
        return toolkit::make_error("failed to parse status line: {}", res.error());

    ParseHeaders(headers_stream, response.Headers);

    auto content_length = ~size_t();
    if (auto it = response.Headers.find("content-length"); it != response.Headers.end())
        if (auto res = toolkit::parse_string<size_t>(it->second) >> content_length; !res)
            return res;

    if (response.Body)
        response.Body->write(body_prefetch.data(), static_cast<long>(body_prefetch.size()));

    auto count = body_prefetch.size();
    while (content_length == ~size_t() || count < content_length)
    {
        auto len = Read(fd, chunk);
        if (len <= 0)
            break;

        if (response.Body)
            response.Body->write(chunk, len);

        count += len;
    }

    return {};
}

toolkit::result<> http::Client::FetchWithRedirects(HttpRequest request, HttpResponse &response)
{
    bool is_redirect;

    do
    {
        if (auto res = Fetch(request, response); !res)
            return res;

        is_redirect = IsRedirect(response.StatusCode);

        if (!is_redirect)
            continue;

        const auto it = response.Headers.find("location");
        if (it == response.Headers.end())
            return toolkit::make_error("missing location header in redirect response.");

        const auto &location = it->second;

        if (location.find("://") != std::string::npos)
            request.Location = ParseURL(location);
        else if (location.starts_with("/"))
            request.Location.Pathname = location;
        else
            request.Location.Pathname += location;

        std::cerr << "redirect to " << location << " --> " << request.Location << std::endl;
    }
    while (is_redirect);

    return {};
}

int http::Client::Read(const int fd, const std::span<char> buffer)
{
    return m_Transport.recv(fd, buffer.data(), buffer.size(), 0);
}

int http::Client::Write(const int fd, const std::span<const char> buffer)
{
    return m_Transport.send(fd, buffer.data(), buffer.size(), 0);
}

toolkit::result<> http::Client::ReadUntil(const int fd, std::string &dst, const char *delimiter)
{
    char chunk[1024];

    while (dst.find(delimiter) == std::string::npos)
    {
        const auto len = Read(fd, chunk);
        if (len <= 0)
            return toolkit::make_error("failed to read chunk.");

        dst.insert(dst.end(), chunk, chunk + len);
    }

    return {};
}
