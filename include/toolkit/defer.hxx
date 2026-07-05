#pragma once

#include <functional>
#include <tuple>

namespace toolkit
{
    template<typename F, typename... A>
    class defer_t
    {
    public:
        defer_t(F &&f, A &&... a)
            : f(std::forward<F>(f)),
              a(std::forward<A>(a)...)
        {
        }

        defer_t(const defer_t &) = delete;
        defer_t &operator=(const defer_t &) = delete;

        defer_t(defer_t &&other) noexcept
            : f(std::move(other.f)),
              a(std::move(other.a))
        {
        }

        defer_t &operator=(defer_t &&other) noexcept
        {
            std::swap(f, other.f);
            std::swap(a, other.a);

            return *this;
        }

        ~defer_t()
        {
            if (active)
                std::apply(f, a);
        }

        void deactivate()
        {
            active = false;
        }

    private:
        bool active = true;

        F f;
        std::tuple<A...> a;
    };

    template<typename F, typename... A>
    defer_t<F, A...> defer(F &&f, A &&... a)
    {
        return { std::forward<F>(f), std::forward<A>(a)... };
    }
}
