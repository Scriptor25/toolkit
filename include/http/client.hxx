#pragma once

#include <http/http.hxx>

#include <toolkit/result.hxx>

#include <memory>
#include <span>

namespace http
{
    struct transport
    {
        virtual ~transport() = default;

        virtual toolkit::result<int> open(const url &location) = 0;
        virtual void close(int fd) = 0;

        virtual int send(int fd, const void *buffer, size_t count) = 0;
        virtual int recv(int fd, void *buffer, size_t count) = 0;
    };

    std::unique_ptr<transport> create_default_transport(bool tls);

    class client
    {
    public:
        explicit client(transport &t);

        [[nodiscard]] toolkit::result<> fetch(request_t request, response_t &response) const;
        [[nodiscard]] toolkit::result<> fetch_with_redirects(request_t request, response_t &response) const;

    private:
        [[nodiscard]] int read(int fd, std::span<char> buffer) const;
        [[nodiscard]] int write(int fd, std::span<const char> buffer) const;

        [[nodiscard]] toolkit::result<> read_until(int fd, std::string &dst, const char *delimiter) const;

        transport &transport_;
    };
}
