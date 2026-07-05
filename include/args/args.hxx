#pragma once

#include <toolkit/result.hxx>

#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace args
{
    enum class entry_kind
    {
        flag,
        value,
        array,
    };

    struct entry
    {
        std::string_view id;
        entry_kind kind;
        std::unordered_set<std::string_view> patterns;
    };

    class manifest
    {
    public:
        manifest() = default;
        manifest(std::initializer_list<entry> entries);

        [[nodiscard]] toolkit::result<> insert(entry e);
        [[nodiscard]] toolkit::result<> insert(std::span<const entry> e);

        [[nodiscard]] const entry *find(std::string_view pattern) const;

    private:
        std::vector<entry> entries_;
        std::unordered_map<std::string_view, const entry *> lookup_;
    };

    class context
    {
        [[nodiscard]] static toolkit::result<> parse_argument(
            context &ctx,
            const entry &e,
            std::string_view key,
            std::string_view val);

    public:
        [[nodiscard]] static toolkit::result<context> parse(
            const manifest &man,
            int argc,
            const char *const *argv);
        [[nodiscard]] static toolkit::result<context> parse(
            const manifest &man,
            std::span<const char *const> args);
        [[nodiscard]] static toolkit::result<context> parse(
            const manifest &man,
            std::span<const std::string_view> args);

        [[nodiscard]] std::string_view file() const;

        [[nodiscard]] bool limited() const;
        [[nodiscard]] size_t limit() const;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] const std::string_view &operator[](size_t index) const;

        [[nodiscard]] std::vector<std::string_view>::const_iterator begin() const;
        [[nodiscard]] std::vector<std::string_view>::const_iterator end() const;

        [[nodiscard]] bool is(std::string_view key) const;

        [[nodiscard]] std::optional<std::string_view> get(std::string_view key) const;
        [[nodiscard]] std::span<const std::string_view> get_all(std::string_view key) const;

        // TODO: add function to generate hash over all flags, values and positional arguments

    private:
        std::string_view file_;
        size_t limit_ = ~size_t();
        std::vector<std::string_view> positional_;
        std::unordered_set<std::string_view> flags_;
        std::unordered_map<std::string_view, std::vector<std::string_view>> values_;
    };
}
