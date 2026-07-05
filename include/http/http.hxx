#pragma once

#include <http/url.hxx>

#include <toolkit/result.hxx>

#include <format>
#include <map>
#include <string>

namespace http
{
    constexpr auto EOL = "\r\n";
    constexpr auto EOL2 = "\r\n\r\n";

    enum class method
    {
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
    };

    enum class status_code : int
    {
        continue_           = 100,
        switching_protocols = 101,

        ok                            = 200,
        created                       = 201,
        accepted                      = 202,
        non_authoritative_information = 203,
        no_content                    = 204,
        reset_connection              = 205,
        partial_content               = 206,

        multiple_choices   = 300,
        moved_permanently  = 301,
        found              = 302,
        see_other          = 303,
        not_modified       = 304,
        use_proxy          = 305,
        temporary_redirect = 307,
        permanent_redirect = 308,

        bad_request                   = 400,
        unauthorized                  = 401,
        payment_required              = 402,
        forbidden                     = 403,
        not_found                     = 404,
        method_not_allowed            = 405,
        not_acceptable                = 406,
        proxy_authentication_required = 407,
        request_timeout               = 408,
        conflict                      = 409,
        gone                          = 410,
        length_required               = 411,
        precondition_failed           = 412,
        content_too_large             = 413,
        uri_too_long                  = 414,
        unsupported_media_type        = 415,
        range_not_satisfiable         = 416,
        expectation_failed            = 417,
        misdirected_request           = 421,
        unprocessable_content         = 422,
        upgrade_required              = 426,

        internal_server_error      = 500,
        not_implemented            = 501,
        bad_gateway                = 502,
        service_unavailable        = 503,
        gateway_timeout            = 504,
        http_version_not_supported = 505,
    };

    inline bool is_directive(status_code code)
    {
        return 100 <= static_cast<int>(code) && static_cast<int>(code) <= 199;
    }

    inline bool is_success(status_code code)
    {
        return 200 <= static_cast<int>(code) && static_cast<int>(code) <= 299;
    }

    inline bool is_redirect(status_code code)
    {
        return 300 <= static_cast<int>(code) && static_cast<int>(code) <= 399;
    }

    inline bool is_client_fail(status_code code)
    {
        return 400 <= static_cast<int>(code) && static_cast<int>(code) <= 499;
    }

    inline bool is_server_fail(status_code code)
    {
        return 500 <= static_cast<int>(code) && static_cast<int>(code) <= 599;
    }

    using headers_t = std::map<std::string, std::string>;

    struct request_t
    {
        method method;
        url location;
        headers_t headers;
        std::istream *body;
    };

    struct response_t
    {
        status_code code;
        std::string message;
        headers_t headers;
        std::ostream *body;
    };

    [[nodiscard]] toolkit::result<> parse_status(
        std::istream &stream,
        status_code &code,
        std::string &message);
    void parse_headers(std::istream &stream, headers_t &headers);
}

std::ostream &operator<<(std::ostream &stream, http::method method);

std::ostream &operator<<(std::ostream &stream, http::status_code code);
std::istream &operator>>(std::istream &stream, http::status_code &code);

template<>
struct std::formatter<http::status_code> : std::formatter<int>
{
    template<typename C>
    auto format(http::status_code status_code, C &&ctx) const
    {
        return std::formatter<int>::format(static_cast<int>(status_code), ctx);
    }
};
