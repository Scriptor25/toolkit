#include <http/client.hxx>

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

namespace
{
    int socket_close(const int fd)
    {
        return close(fd);
    }

    struct default_transport : http::transport
    {
        toolkit::result<int> open(const http::url &location) override
        {
            if (location.scheme != "http" && location.scheme != "https")
                return toolkit::make_error("unsupported scheme '{}'", location.scheme);

            const auto service = std::to_string(location.port);

            const addrinfo hints
            {
                .ai_family = AF_UNSPEC,
                .ai_socktype = SOCK_STREAM,
                .ai_protocol = 0,
            };

            addrinfo *info{};
            if (auto error = getaddrinfo(location.host.c_str(), service.c_str(), &hints, &info))
            {
                return toolkit::make_error("failed to get address info ({}).", error);
            }

            auto fd = -1;

            for (auto it = info; it; it = it->ai_next)
            {
                fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
                if (fd < 0)
                {
                    continue;
                }

                if (connect(fd, it->ai_addr, it->ai_addrlen))
                {
                    socket_close(fd);
                    fd = -1;
                    continue;
                }

                break;
            }

            freeaddrinfo(info);

            if (fd < 0)
                return toolkit::make_error("failed to open socket.");

            return fd;
        }

        void close(const int fd) override
        {
            socket_close(fd);
        }

        int send(const int fd, const void *buffer, const size_t count) override
        {
            return ::send(fd, buffer, count, 0);
        }

        int recv(const int fd, void *buffer, const size_t count) override
        {
            return ::recv(fd, buffer, count, 0);
        }
    };
}

std::unique_ptr<http::transport> http::create_default_tcp_transport()
{
    return std::make_unique<default_transport>();
}
