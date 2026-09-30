#include <http/client.hxx>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

static int socket_close(const int fd)
{
    return closesocket(fd);
}

#endif

#if defined(__linux__) || defined(__APPLE__)

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>

static int socket_close(const int fd)
{
    return close(fd);
}

#endif

namespace
{
    struct default_transport : http::transport
    {
        default_transport()
        {
#ifdef _WIN32
            WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        }

        ~default_transport() override
        {
#ifdef _WIN32
            WSACleanup();
#endif
        }

        default_transport(const default_transport &) = delete;
        default_transport &operator=(const default_transport &) = delete;

        default_transport(default_transport &&other) noexcept
        {
#ifdef _WIN32
            std::swap(wsa, other.wsa)
#endif
        }

        default_transport &operator=(default_transport &&other) noexcept
        {
#ifdef _WIN32
            std::swap(wsa, other.wsa)
#endif

            return *this;
        }

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

#ifdef _WIN32
        WSADATA wsa{};
#endif
    };
}

std::unique_ptr<http::transport> http::create_default_tcp_transport()
{
    return std::make_unique<default_transport>();
}
