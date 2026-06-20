#pragma once

#include <toolkit/result.hxx>

#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace toolkit
{
    enum class arg_kind
    {
        flag,
        value,
        array,
    };

    struct arg_entry
    {
        std::string_view id;
        arg_kind kind;
        std::unordered_set<std::string_view> patterns;
    };

    struct arg_manifest
    {
        arg_manifest(std::vector<arg_entry> arg_entries);

        [[nodiscard]] const arg_entry *find(std::string_view pattern) const;

        std::vector<arg_entry> entries;
        std::unordered_map<std::string_view, const arg_entry *> lookup;
    };

    struct arg_context
    {
        [[nodiscard]] bool is(std::string_view key) const;
        [[nodiscard]] std::optional<std::string_view> get(std::string_view key) const;
        [[nodiscard]] std::span<const std::string_view> get_all(std::string_view key) const;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] const std::string_view &operator[](size_t index) const;

        [[nodiscard]] std::vector<std::string_view>::const_iterator begin() const;
        [[nodiscard]] std::vector<std::string_view>::const_iterator end() const;

        std::string_view file;
        size_t limit;
        std::vector<std::string_view> positional;
        std::unordered_set<std::string_view> flags;
        std::unordered_map<std::string_view, std::vector<std::string_view>> values;
    };

    [[nodiscard]] result<arg_context> arg_parse(const arg_manifest &manifest, int argc, const char *const*argv);
}
