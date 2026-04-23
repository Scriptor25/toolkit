#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <variant>

namespace data
{
    template<typename, typename T>
    struct copy_cv
    {
        using type = T;
    };

    template<typename F, typename T>
    struct copy_cv<const F, T>
    {
        using type = std::add_const_t<T>;
    };

    template<typename F, typename T>
    struct copy_cv<volatile F, T>
    {
        using type = std::add_volatile_t<T>;
    };

    template<typename F, typename T>
    struct copy_cv<const volatile F, T>
    {
        using type = std::add_cv_t<T>;
    };

    template<typename F, typename T>
    using copy_cv_t = copy_cv<F, T>::type;

    template<typename F, typename T>
    struct copy_cvref
    {
        using type = copy_cv_t<std::remove_reference_t<F>, T>;
    };

    template<typename F, typename T>
    struct copy_cvref<F &, T>
    {
        using type = std::add_lvalue_reference_t<copy_cv_t<std::remove_reference_t<F>, T>>;
    };

    template<typename F, typename T>
    struct copy_cvref<F &&, T>
    {
        using type = std::add_rvalue_reference_t<copy_cv_t<std::remove_reference_t<F>, T>>;
    };

    template<typename F, typename T>
    using copy_cvref_t = copy_cvref<F, T>::type;

    template<typename, typename>
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
    concept is_result_v = is_result<T>::value;

    template<typename E>
    struct error_result
    {
        E value;
    };

    template<typename T, typename E = std::string>
    class result
    {
    public:
        using value_type = std::decay_t<T>;
        using error_type = error_result<std::decay_t<E>>;

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
            std::swap(container, other.container);
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
            throw std::runtime_error(std::get<error_type>(container).value);
        }

        auto &&operator*() const &
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return *ptr;
            throw std::runtime_error(std::get<error_type>(container).value);
        }

        auto &&operator*() &&
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return std::move(*ptr);
            throw std::runtime_error(std::get<error_type>(container).value);
        }

        auto *operator->() &
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return ptr;
            throw std::runtime_error(std::get<error_type>(container).value);
        }

        auto *operator->() const &
        {
            if (const auto ptr = std::get_if<value_type>(&container))
                return ptr;
            throw std::runtime_error(std::get<error_type>(container).value);
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

        auto &&value() const &&
        {
            return std::get<value_type>(std::move(container));
        }

        auto &&error() &
        {
            return std::get<error_type>(container).value;
        }

        auto &&error() const &
        {
            return std::get<error_type>(container).value;
        }

        auto &&error() &&
        {
            return std::get<error_type>(std::move(container)).value;
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
            static_assert(is_result_v<R>);

            if (auto *ptr = std::get_if<value_type>(&s.value))
                return std::forward<F>(f)(std::forward<A>(*ptr));

            return R(std::forward<S>(std::get<error_type>(s.value)));
        }

        template<typename S, typename F>
        static auto or_else_impl(S &&s, F &&f)
        {
            using A = copy_cvref_t<S, error_type>;
            using R = std::invoke_result_t<F &&, A>;
            static_assert(is_result_v<R>);

            if (auto *ptr = std::get_if<error_type>(&s.value))
                return std::forward<F>(f)(std::forward<A>(*ptr));

            return R(std::forward<S>(std::get<value_type>(s.value)));
        }

        std::variant<value_type, error_type> container;
    };

    template<typename... A>
    auto make_error(std::format_string<A...> fmt, A &&... args)
    {
        return error_result{ std::format(std::move(fmt), std::forward<A>(args)...) };
    }
}
