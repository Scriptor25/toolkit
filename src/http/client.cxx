#include <http/client.hxx>
#include <http/http.hxx>
#include <http/url.hxx>

#include <toolkit/defer.hxx>
#include <toolkit/result.hxx>
#include <toolkit/string.hxx>

#include <charconv>
#include <ios>
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

namespace
{
    class buffered_reader
    {
    public:
        buffered_reader(http::transport &transport, int fd)
            : transport_(transport), fd_(fd)
        {
        }

        toolkit::result<std::string> read_line()
        {
            std::string line;

            while (true)
            {
                const auto end = buffer_.find("\r\n", offset_);
                if (end != std::string::npos)
                {
                    line.append(buffer_, offset_, end - offset_);
                    offset_ = end + 2;
                    compact();
                    return line;
                }

                if (offset_ < buffer_.size())
                {
                    line.append(buffer_, offset_, buffer_.size() - offset_);
                    offset_ = buffer_.size();
                }

                if (auto res = fill(); !res)
                    return res;

                if (eof_)
                    return toolkit::make_error("unexpected EOF while reading line.");
            }
        }

        toolkit::result<> read_exact(size_t count, std::ostream *destination)
        {
            while (count > 0)
            {
                if (available() == 0)
                {
                    if (auto res = fill(); !res)
                        return res;

                    if (eof_)
                        return toolkit::make_error("unexpected EOF while reading body.");
                }

                const auto len = std::min(count, available());

                if (destination)
                {
                    destination->write(buffer_.data() + offset_, static_cast<std::streamsize>(len));

                    if (!*destination)
                        return toolkit::make_error("failed to write response body.");
                }

                offset_ += len;
                count -= len;
                compact();
            }

            return {};
        }

        toolkit::result<> read_to_eof(std::ostream *destination)
        {
            while (true)
            {
                const auto len = available();

                if (len > 0)
                {
                    if (destination)
                    {
                        destination->write(buffer_.data() + offset_, static_cast<std::streamsize>(len));

                        if (!*destination)
                            return toolkit::make_error("failed to write response body.");
                    }

                    offset_ = buffer_.size();
                    compact();
                    continue;
                }

                if (auto res = fill(); !res)
                    return res;

                if (eof_)
                    return {};
            }
        }

        toolkit::result<> read_chunked(std::ostream *destination)
        {
            std::string line;

            while (true)
            {
                if (auto res = read_line() >> line; !res)
                    return res;

                const auto semicolon = line.find(';');
                const std::string_view size_text(line.data(), semicolon == std::string::npos ? line.size() : semicolon);

                if (size_text.empty())
                    return toolkit::make_error("empty chunk size.");
                
                size_t chunk_size;
                const auto [end, ec] = std::from_chars(size_text.data(), size_text.data() + size_text.size(), chunk_size, 16);

                if (ec != std::errc() || end != size_text.data() + size_text.size())
                    return toolkit::make_error("invalid chunk size '{}'.", size_text);

                if (chunk_size == 0)
                {
                    while (true)
                    {
                        if (auto res = read_line() >> line; !res)
                            return res;
                        
                        if (line.empty())
                            return {};
                    }
                }

                if (auto res = read_exact(chunk_size, destination); !res)
                    return res;

                if (auto res = read_line() >> line; !res)
                    return res;
                
                if (!line.empty())
                    return toolkit::make_error("missing CRLF after chunk data.");
            }
        }
    
    private:
        size_t available() const
        {
            return buffer_.size() - offset_;
        }

        toolkit::result<> fill()
        {
            if (eof_)
                return {};
            
            std::array<char, 4096> chunk{};

            const auto len = transport_.recv(fd_, chunk.data(), chunk.size());

            if (len < 0)
                return toolkit::make_error("failed to read data.");

            if (len == 0)
            {
                eof_ = true;
                return {};
            }

            if (static_cast<size_t>(len) > chunk.size())
                return toolkit::make_error("invalid read result.");

            buffer_.append(chunk.data(), static_cast<size_t>(len));
            return {};
        }

        void compact()
        {
            if (offset_ == buffer_.size())
            {
                buffer_.clear();
                offset_ = 0;
            }
            else if (offset_ > 4096)
            {
                buffer_.erase(0, offset_);
                offset_ = 0;
            }
        }

    private:
        http::transport &transport_;
        int fd_;

        std::string buffer_;
        size_t offset_ = 0;
        bool eof_ = false;
    };
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
        return toolkit::make_error("failed to write request headers: {}", res.error());

    if (request.body)
    {
        std::array<char, 4096> chunk{};

        while (true)
        {
            request.body->read(chunk.data(), chunk.size());
            const auto count = request.body->gcount();

            if (count > 0)
            {
                if (auto res = write_all(chunk, [this, fd](auto data) { return write(fd, data); }); !res)
                    return toolkit::make_error("failed to write request body: {}", res.error());
            }

            if (request.body->eof())
                break;

            if (!*request.body)
                return toolkit::make_error("failed to read request body.");
        }
    }

    buffered_reader reader(transport_, fd);

    while (true)
    {
        std::string status_line;
        if (auto res = reader.read_line() >> status_line; !res)
            return toolkit::make_error("failed to read status line: {}", res.error());

        std::istringstream status_stream(status_line + EOL);
        if (auto res = parse_status(status_stream, response.code, response.message); !res)
            return toolkit::make_error("failed to parse status line: {}", res.error());

        std::string headers_text;
        std::string line;

        while (true)
        {
            if (auto res = reader.read_line() >> line; !res)
                return toolkit::make_error("failed to read response headers: {}", res.error());

            if (line.empty())
                break;

            headers_text += line;
            headers_text += EOL;
        }

        headers_text += EOL;

        std::istringstream headers_stream(headers_text);
        if (auto res = parse_headers(headers_stream, response.headers); !res)
            return toolkit::make_error("failed to parse headers: {}", res.error());

        if (!is_directive(response.code) || response.code == status_code::switching_protocols)
            break;
    }

    const auto status = static_cast<int>(response.code);

    const auto body_forbidden = request.method == method::head || (status >= 100 && status < 200) || response.code == status_code::no_content || response.code == status_code::reset_connection || response.code == status_code::not_modified;

    if (body_forbidden)
        return {};

    const auto transfer_it = response.headers.find("transfer-encoding");
    const auto length_it = response.headers.find("content-length");

    if (transfer_it != response.headers.end())
    {
        const auto encoding = toolkit::lowercase(toolkit::trim(transfer_it->second));

        if (encoding != "chunked")
            return toolkit::make_error("unsupported transfer-encoding '{}'.", encoding);

        if (length_it != response.headers.end())
            return toolkit::make_error("response contains both transfer-encoding and content-length headers.");

        if (auto res = reader.read_chunked(response.body); !res)
            return toolkit::make_error("failed to read chunked response body: {}", res.error());

        return {};
    }

    if (length_it != response.headers.end())
    {
        size_t content_length = 0;
        const auto &text = length_it->second;

        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), content_length);

        if (ec != std::errc() || end != text.data() + text.size())
            return toolkit::make_error("invalid content-length header '{}'.", text);

        if (auto res = reader.read_exact(content_length, response.body); !res)
            return toolkit::make_error("failed to read response body: {}", res.error());

        return {};
    }

    if (auto res = reader.read_to_eof(response.body); !res)
        return toolkit::make_error("failed to read response body: {}", res.error());

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
