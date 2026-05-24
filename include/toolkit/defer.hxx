#pragma once

#include <functional>
#include <tuple>

namespace toolkit
{
    template<typename F, typename... A>
    struct deferred
    {
        deferred(F &&f, A &&... a)
            : f(std::forward<F>(f)),
              a(std::forward<A>(a)...)
        {
        }

        deferred(const deferred &) = delete;
        deferred &operator=(const deferred &) = delete;

        deferred(deferred &&other) noexcept
            : f(std::move(other.f)),
              a(std::move(other.a))
        {
        }

        deferred &operator=(deferred &&other) noexcept
        {
            std::swap(f, other.f);
            std::swap(a, other.a);

            return *this;
        }

        ~deferred()
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
    deferred<F, A...> defer(F &&f, A &&... a)
    {
        return { std::forward<F>(f), std::forward<A>(a)... };
    }
}
