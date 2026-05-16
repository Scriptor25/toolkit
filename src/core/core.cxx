#include <toolkit/args.hxx>
#include <toolkit/string.hxx>

#include <iostream>

const toolkit::arg_entry *toolkit::arg_manifest::find(const std::string_view &pattern) const
{
    for (auto &entry : entries)
        if (entry.patterns.contains(pattern))
            return &entry;

    return nullptr;
}

bool toolkit::arg_context::is(const std::string_view key) const
{
    return flags.contains(key);
}

bool toolkit::arg_context::get(const std::string_view key, std::string_view &value) const
{
    if (const auto it = values.find(key); it != values.end())
    {
        value = it->second.front();
        return true;
    }

    return false;
}

void toolkit::arg_context::get_all(const std::string_view key, std::vector<std::string_view> &value) const
{
    if (const auto it = values.find(key); it != values.end())
    {
        value = it->second;
        return;
    }

    value.clear();
}

void toolkit::arg_parse(arg_context &context, const arg_manifest &manifest, const int argc, const char *const *argv)
{
    const std::vector<std::string_view> args(argv, argv + argc);

    context.file = args[0];

    std::vector<std::string_view> undefined;

    for (size_t i = 1; i < args.size(); ++i)
    {
        auto &arg = args[i];

        if (auto *entry = manifest.find(arg))
        {
            if (entry->is_flag)
            {
                context.flags.insert(entry->id);
                continue;
            }

            auto &val = args[++i];

            auto &dst = context.values[entry->id];

            if (entry->is_array)
            {
                std::vector<std::string_view> vals;
                split(vals, val, ',');

                dst.insert(dst.end(), vals.begin(), vals.end());
                continue;
            }

            dst = { val };
            continue;
        }

        if (const auto pos = arg.find('='); pos != std::string::npos)
        {
            auto key = arg.substr(0, pos);

            if (auto *entry = manifest.find(key))
            {
                auto val = arg.substr(pos + 1);

                auto &dst = context.values[entry->id];

                if (entry->is_array)
                {
                    std::vector<std::string_view> vals;
                    split(vals, val, ',');

                    dst.insert(dst.end(), vals.begin(), vals.end());
                    continue;
                }

                dst = { val };
                continue;
            }
        }

        undefined.push_back(arg);
    }

    if (undefined.empty())
        return;

    std::cerr << "undefined arguments:\n";
    for (const auto arg : undefined)
        std::cerr << " - " << arg << '\n';
    std::cerr << std::endl;
}
