#include <http/client.hxx>

#include <openssl/ssl.h>

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
        explicit default_transport(bool tls)
        {
#ifdef _WIN32
            WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

            if (tls)
            {
                ctx = SSL_CTX_new(TLS_client_method());
                SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
                SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
                SSL_CTX_set_default_verify_paths(ctx);
            }
        }

        ~default_transport() override
        {
            if (ctx)
                SSL_CTX_free(ctx);

#ifdef _WIN32
            WSACleanup();
#endif
        }

        default_transport(const default_transport &) = delete;
        default_transport &operator=(const default_transport &) = delete;

        default_transport(default_transport &&other) noexcept
        {
#ifdef _WIN32
            std::swap(wsa, other.wsa);
#endif

            std::swap(ctx, other.ctx);
            std::swap(ssl, other.ssl);
        }

        default_transport &operator=(default_transport &&other) noexcept
        {
#ifdef _WIN32
            std::swap(wsa, other.wsa);
#endif

            std::swap(ctx, other.ctx);
            std::swap(ssl, other.ssl);

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
                return toolkit::make_error("failed to get address info ({}).", error);

            auto fd = -1;

            for (auto it = info; it; it = it->ai_next)
            {
                fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
                if (fd < 0)
                    continue;

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

            if (ctx && location.scheme == "https")
            {
                auto *s = SSL_new(ctx);
                SSL_set_fd(s, fd);

                SSL_set_tlsext_host_name(s, location.host.c_str());
                SSL_set1_host(s, location.host.c_str());
                SSL_set_verify(s, SSL_VERIFY_PEER, nullptr);

                if (SSL_connect(s) <= 0)
                    return toolkit::make_error("TLS handshake failed.");

                if (SSL_get_verify_result(s) != X509_V_OK)
                    return toolkit::make_error("TLS certificate verification failed.");

                ssl[fd] = s;
            }

            return fd;
        }

        void close(const int fd) override
        {
            if (auto *s = ssl[fd])
                SSL_free(s);
            socket_close(fd);
        }

        int send(const int fd, const void *buffer, const size_t count) override
        {
            if (auto *s = ssl[fd])
                return SSL_write(s, buffer, static_cast<int>(count));
            return static_cast<int>(::send(fd, buffer, count, 0));
        }

        int recv(const int fd, void *buffer, const size_t count) override
        {
            if (auto *s = ssl[fd])
                return SSL_read(s, buffer, static_cast<int>(count));
            return static_cast<int>(::recv(fd, buffer, count, 0));
        }

#ifdef _WIN32
        WSADATA wsa{};
#endif

        SSL_CTX *ctx{};
        std::unordered_map<int, SSL *> ssl;
    };
}

std::unique_ptr<http::transport> http::create_default_transport(bool tls)
{
    return std::make_unique<default_transport>(tls);
}
