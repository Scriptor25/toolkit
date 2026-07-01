#pragma once

#include <http/http.hxx>

#include <toolkit/result.hxx>

namespace http
{
    struct Transport
    {
        int open(const URL &location);
        void close(int fd);

        int send(int fd, const void *buffer, size_t count, int flags);
        int recv(int fd, void *buffer, size_t count, int flags);
    };

    class Client
    {
    public:
        explicit Client(Transport transport);

        [[nodiscard]] toolkit::result<> Fetch(HttpRequest request, HttpResponse &response);
        [[nodiscard]] toolkit::result<> FetchWithRedirects(HttpRequest request, HttpResponse &response);

    private:
        int Read(int fd, std::span<char> buffer);
        int Write(int fd, std::span<const char> buffer);

        [[nodiscard]] toolkit::result<> ReadUntil(int fd, std::string &dst, const char *delimiter);

        Transport m_Transport;
    };
}
