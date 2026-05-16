#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace toolkit
{
    struct arg_entry
    {
        std::string_view id;
        std::unordered_set<std::string_view> patterns;
        bool is_flag;
        bool is_array;
    };

    struct arg_manifest
    {
        [[nodiscard]] const arg_entry *find(const std::string_view &pattern) const;

        std::vector<arg_entry> entries;
    };

    struct arg_context
    {
        bool is(std::string_view key) const;
        bool get(std::string_view key, std::string_view &value) const;
        void get_all(std::string_view key, std::vector<std::string_view> &value) const;

        std::string_view file;
        std::unordered_set<std::string_view> flags;
        std::unordered_map<std::string_view, std::vector<std::string_view>> values;
    };

    void arg_parse(arg_context &context, const arg_manifest &manifest, int argc, const char *const*argv);
}
