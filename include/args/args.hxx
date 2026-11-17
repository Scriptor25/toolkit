#pragma once

#include <toolkit/result.hxx>

#include <optional>
#include <ostream>
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
        std::string id;
        entry_kind kind;

        std::unordered_set<std::string> patterns;

        std::string description;
    };

    class manifest
    {
    public:
        static toolkit::result<manifest> from(std::initializer_list<entry> entries);

        manifest() = default;

        [[nodiscard]] toolkit::result<> insert(entry e);
        [[nodiscard]] toolkit::result<> insert(std::span<const entry> es);

        [[nodiscard]] const entry *find(const std::string &pattern) const;

        void describe(std::ostream &stream) const;

    private:
        std::vector<entry> entries_;
        std::unordered_set<std::string> ids_;
        std::unordered_map<std::string, std::size_t> lookup_;
    };

    class context
    {
        [[nodiscard]] static toolkit::result<> parse_argument(
            context &ctx,
            const entry &e,
            const std::string &key,
            const std::string &val);

        [[nodiscard]] static bool parse_flags(
            const manifest &man,
            const std::string &arg,
            context *ctx = {});

        [[nodiscard]] static bool is_option(
            const manifest &man,
            const std::string &arg);

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
            std::span<const std::string> args);

        [[nodiscard]] const std::string &file() const;

        [[nodiscard]] bool limited() const;
        [[nodiscard]] std::size_t limit() const;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] const std::string &operator[](std::size_t index) const;

        [[nodiscard]] std::vector<std::string>::const_iterator begin() const;
        [[nodiscard]] std::vector<std::string>::const_iterator end() const;

        [[nodiscard]] bool is(const std::string &key) const;
        [[nodiscard]] std::size_t count(const std::string &key) const;

        [[nodiscard]] std::optional<std::string> get(const std::string &key) const;
        [[nodiscard]] std::span<const std::string> get_all(const std::string &key) const;

    private:
        std::string file_;
        std::size_t limit_ = ~std::size_t();
        std::vector<std::string> positional_;
        std::unordered_map<std::string, std::size_t> flags_;
        std::unordered_map<std::string, std::vector<std::string>> values_;
    };
}
