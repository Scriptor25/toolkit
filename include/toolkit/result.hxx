#pragma once

#include <toolkit/templates.hxx>

#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

namespace toolkit
{
    template<typename = void, typename = std::string>
    class result;

    template<typename>
    struct is_result : std::false_type
    {
    };

    template<typename T, typename E>
    struct is_result<result<T, E>> : std::true_type
    {
    };

    template<typename T>
    concept result_type = is_result<T>::value;

    template<typename E>
    struct result_error
    {
        E message;
    };

    template<typename E>
    class result<void, E>
    {
        template<typename, typename>
        friend class result;

    public:
        using value_type = std::monostate;
        using error_value_type = std::decay_t<E>;
        using error_type = result_error<error_value_type>;

        result()
            : container(value_type{})
        {
        }

        result(const error_type &error)
            : container(error)
        {
        }

        result(error_type &&error)
            : container(std::move(error))
        {
        }

        result(const result &other)
            : container(other.container)
        {
        }

        result(result &&other) noexcept
            : container(std::move(other.container))
        {
        }

        template<result_type R>
        result(R &&r)
            : container(std::get<error_type>(std::forward<R>(r).container))
        {
        }

        result &operator=(const error_type &error)
        {
            container = error;
            return *this;
        }

        result &operator=(error_type &&error)
        {
            container = std::move(error);
            return *this;
        }

        result &operator=(const result &other)
        {
            container = other.container;
            return *this;
        }

        result &operator=(result &&other) noexcept
        {
            container = std::move(other.container);
            return *this;
        }

        template<result_type R>
        result &operator=(R &&r)
        {
            container = std::get<error_type>(std::forward<R>(r).container);
            return *this;
        }

        explicit operator bool() const
        {
            return std::holds_alternative<value_type>(container);
        }

        bool operator!() const
        {
            return std::holds_alternative<error_type>(container);
        }

        bool operator<=>(const result &other) const
        {
            return container <=> other.container;
        }

        auto &&error() &
        {
            return std::get<error_type>(container).message;
        }

        auto &&error() const &
        {
            return std::get<error_type>(container).message;
        }

        auto &&error() &&
        {
            return std::get<error_type>(std::move(container)).message;
        }

        template<typename F>
        auto and_then(F &&f) &
        {
            return and_then_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto and_then(F &&f) const &
        {
            return and_then_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto and_then(F &&f) &&
        {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) &
        {
            return or_else_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) const &
        {
            return or_else_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) &&
        {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }

    private:
        template<typename S, typename F>
        static auto and_then_impl(S &&s, F &&f)
        {
            using R = std::invoke_result_t<F &&>;
            static_assert(result_type<R>);

            if (std::holds_alternative<value_type>(s.container))
                return std::forward<F>(f)();

            return R(std::get<error_type>(std::forward<S>(s).container));
        }

        template<typename S, typename F>
        static auto or_else_impl(S &&s, F &&f)
        {
            using A = copy_cvref_t<S, error_value_type>;
            using R = std::invoke_result_t<F &&, A>;
            static_assert(result_type<R>);

            if (auto *ptr = std::get_if<error_type>(&s.container))
                return std::forward<F>(f)(std::forward<A>(ptr->message));

            return R();
        }

        std::variant<value_type, error_type> container;
    };

    template<typename T, typename E>
    class result
    {
        template<typename, typename>
        friend class result;

    public:
        using value_type = std::decay_t<T>;
        using error_value_type = std::decay_t<E>;
        using error_type = result_error<error_value_type>;

        result()
            : container(value_type{})
        {
        }

        result(const value_type &value)
            : container(value)
        {
        }

        result(value_type &&value)
            : container(std::move(value))
        {
        }

        result(const error_type &error)
            : container(error)
        {
        }

        result(error_type &&error)
            : container(std::move(error))
        {
        }

        result(const result &other)
            : container(other.container)
        {
        }

        result(result &&other) noexcept
            : container(std::move(other.container))
        {
        }

        template<result_type R>
        result(R &&r)
            : container(std::get<error_type>(std::forward<R>(r).container))
        {
        }

        result &operator=(const value_type &value)
        {
            container = value;
            return *this;
        }

        result &operator=(value_type &&value)
        {
            container = std::move(value);
            return *this;
        }

        result &operator=(const error_type &error)
        {
            container = error;
            return *this;
        }

        result &operator=(error_type &&error)
        {
            container = std::move(error);
            return *this;
        }

        result &operator=(const result &other)
        {
            container = other.container;
            return *this;
        }

        result &operator=(result &&other) noexcept
        {
            container = std::move(other.container);
            return *this;
        }

        template<result_type R>
        result &operator=(R &&r)
        {
            container = std::get<error_type>(std::forward<R>(r).container);
            return *this;
        }

        explicit operator bool() const
        {
            return std::holds_alternative<value_type>(container);
        }

        bool operator!() const
        {
            return std::holds_alternative<error_type>(container);
        }

        bool operator<=>(const result &other) const
        {
            return container <=> other.container;
        }

        auto &&operator*() &
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return *ptr;
            throw std::runtime_error(std::get<error_type>(container).message);
        }

        auto &&operator*() const &
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return *ptr;
            throw std::runtime_error(std::get<error_type>(container).message);
        }

        auto &&operator*() &&
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return std::move(*ptr);
            throw std::runtime_error(std::get<error_type>(container).message);
        }

        auto *operator->()
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return ptr;
            throw std::runtime_error(std::get<error_type>(container).message);
        }

        auto *operator->() const
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return ptr;
            throw std::runtime_error(std::get<error_type>(container).message);
        }

        auto &&value() &
        {
            return std::get<value_type>(container);
        }

        auto &&value() const &
        {
            return std::get<value_type>(container);
        }

        auto &&value() &&
        {
            return std::get<value_type>(std::move(container));
        }

        auto &&error() &
        {
            return std::get<error_type>(container).message;
        }

        auto &&error() const &
        {
            return std::get<error_type>(container).message;
        }

        auto &&error() &&
        {
            return std::get<error_type>(std::move(container)).message;
        }

        template<typename F>
        auto and_then(F &&f) &
        {
            return and_then_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto and_then(F &&f) const &
        {
            return and_then_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto and_then(F &&f) &&
        {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) &
        {
            return or_else_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) const &
        {
            return or_else_impl(*this, std::forward<F>(f));
        }

        template<typename F>
        auto or_else(F &&f) &&
        {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }

    private:
        template<typename S, typename F>
        static auto and_then_impl(S &&s, F &&f)
        {
            using A = copy_cvref_t<S, value_type>;
            using R = std::invoke_result_t<F &&, A>;
            static_assert(result_type<R>);

            if (auto *ptr = std::get_if<value_type>(&s.container))
                return std::forward<F>(f)(std::forward<A>(*ptr));

            return R(std::get<error_type>(std::forward<S>(s).container));
        }

        template<typename S, typename F>
        static auto or_else_impl(S &&s, F &&f)
        {
            using A = copy_cvref_t<S, error_value_type>;
            using R = std::invoke_result_t<F &&, A>;
            static_assert(result_type<R>);

            if (auto *ptr = std::get_if<error_type>(&s.container))
                return std::forward<F>(f)(std::forward<A>(ptr->message));

            return R(std::get<value_type>(std::forward<S>(s).container));
        }

        std::variant<value_type, error_type> container;
    };

    template<typename... A>
    auto make_error(std::format_string<A...> fmt, A &&... args)
    {
        return result_error{ std::format(std::move(fmt), std::forward<A>(args)...) };
    }

    template<result_type R>
    auto operator>>(R &&r, typename R::value_type &v)
    {
        return std::forward<R>(r).and_then(
            [&v]<typename T>(T &&value) -> result<>
            {
                v = std::forward<T>(value);
                return {};
            });
    }

    template<result_type R, typename F>
    auto operator&(R &&r, F &&f)
    {
        return std::forward<R>(r).and_then(std::forward<F>(f));
    }

    template<result_type R, typename F>
    auto operator|(R &&r, F &&f)
    {
        return std::forward<R>(r).or_else(std::forward<F>(f));
    }
}
