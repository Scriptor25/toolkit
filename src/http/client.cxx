#include <http/client.hxx>
#include <http/http.hxx>
#include <http/url.hxx>

#include <toolkit/defer.hxx>
#include <toolkit/string.hxx>

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>

http::client::client(transport &t)
    : transport_(t)
{
}

static void set_header_if_missing(http::headers_t &headers, const std::string &key, const std::string &val)
{
    headers.emplace(toolkit::lowercase(key), val);
}

template<typename writefn>
toolkit::result<> write_all(std::span<const char> data, writefn &&write)
{
    while (!data.empty())
    {
        const auto len = write(data);

        if (len < 0)
            return toolkit::make_error("failed to write data.");

        if (len == 0)
            return toolkit::make_error("connection closed.");

        if (static_cast<size_t>(len) > data.size())
            return toolkit::make_error("invalid write result.");

        data = data.subspan(static_cast<size_t>(len));
    }

    return {};
}

toolkit::result<> http::client::fetch(request_t request, response_t &response) const
{
    int fd;
    if (auto res = transport_.open(request.location) >> fd; !res)
        return res;

    auto guard0 = toolkit::defer(
        [this](auto x)
        {
            transport_.close(x);
        },
        fd);

    set_header_if_missing(request.headers, "host", request.location.host);
    set_header_if_missing(request.headers, "connection", "close");
    set_header_if_missing(request.headers, "accept-encoding", "identity");

    std::stringstream packet;
    packet << request.method << ' ' << request.location.pathname << ' ' << "HTTP/1.1" << EOL;
    for (auto &[key, val] : request.headers)
        packet << key << ": " << val << EOL;
    packet << EOL;

    if (auto res = write_all(packet.str(), [this, fd](auto data) { return write(fd, data); }); !res)
        return toolkit::make_error("failed to send header: {}", res.error());

    char chunk[4096];

    if (request.body)
    {
        size_t count = 0;

        while (true)
        {
            request.body->read(chunk, sizeof(chunk));
            const size_t len = request.body->gcount();

            if (len <= 0)
                break;

            if (write(fd, { chunk, len }) < 0)
                return toolkit::make_error("failed to send chunk.");

            count += len;
        }
    }

    std::string header_block;
    if (auto res = read_until(fd, header_block, EOL2); !res)
        return toolkit::make_error("failed to read header block: {}", res.error());

    auto headers_end = header_block.find(EOL2);
    auto headers = header_block.substr(0, headers_end);
    auto body_prefetch = header_block.substr(headers_end + 4);

    std::istringstream headers_stream(headers);

    std::string status_line;
    toolkit::get_line(headers_stream, status_line, EOL);

    std::istringstream status_stream(status_line);
    if (auto res = parse_status(status_stream, response.code, response.message); !res)
        return toolkit::make_error("failed to parse status line: {}", res.error());

    parse_headers(headers_stream, response.headers);

    auto content_length = ~size_t();
    if (auto it = response.headers.find("content-length"); it != response.headers.end())
        if (auto res = toolkit::parse_string<size_t>(it->second) >> content_length; !res)
            return res;

    if (response.body)
        response.body->write(body_prefetch.data(), static_cast<long>(body_prefetch.size()));

    auto count = body_prefetch.size();
    while (content_length == ~size_t() || count < content_length)
    {
        auto len = read(fd, chunk);
        if (len <= 0)
            break;

        if (response.body)
            response.body->write(chunk, len);

        count += len;
    }

    return {};
}

toolkit::result<> http::client::fetch_with_redirects(request_t request, response_t &response) const
{
    bool redirect;

    do
    {
        if (auto res = fetch(request, response); !res)
            return res;

        redirect = is_redirect(response.code);

        if (!redirect)
            continue;

        const auto it = response.headers.find("location");
        if (it == response.headers.end())
            return toolkit::make_error("missing location header in redirect response.");

        const auto &location = it->second;

        if (location.find("://") != std::string::npos)
            request.location = url::parse(location);
        else if (location.starts_with("/"))
            request.location.pathname = location;
        else
            request.location.pathname += location;

        std::cerr << "redirect to " << location << " --> " << request.location << std::endl;
    }
    while (redirect);

    return {};
}

int http::client::read(const int fd, const std::span<char> buffer) const
{
    return transport_.recv(fd, buffer.data(), buffer.size());
}

int http::client::write(const int fd, const std::span<const char> buffer) const
{
    return transport_.send(fd, buffer.data(), buffer.size());
}

toolkit::result<> http::client::read_until(const int fd, std::string &dst, const char *delimiter) const
{
    char chunk[1024];

    while (dst.find(delimiter) == std::string::npos)
    {
        const auto len = read(fd, chunk);
        if (len <= 0)
            return toolkit::make_error("failed to read chunk.");

        dst.insert(dst.end(), chunk, chunk + len);
    }

    return {};
}
