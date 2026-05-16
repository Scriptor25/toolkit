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

bool toolkit::arg_context::empty() const
{
    return positional.empty();
}

size_t toolkit::arg_context::size() const
{
    return positional.size();
}

const std::string_view &toolkit::arg_context::operator[](const size_t index) const
{
    return positional[index];
}

std::vector<std::string_view>::const_iterator toolkit::arg_context::begin() const
{
    return positional.begin();
}

std::vector<std::string_view>::const_iterator toolkit::arg_context::end() const
{
    return positional.end();
}

void toolkit::arg_parse(arg_context &context, const arg_manifest &manifest, const int argc, const char *const *argv)
{
    const std::vector<std::string_view> args(argv, argv + argc);

    context.file = args[0];
    context.positional.clear();
    context.flags.clear();
    context.values.clear();

    for (size_t i = 1; i < args.size(); ++i)
    {
        auto arg = args[i];

        if (auto *entry = manifest.find(arg))
        {
            if (entry->is_flag)
            {
                context.flags.insert(entry->id);
                continue;
            }

            auto val = args[++i];

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

        if (const auto pos = arg.find('='); pos != std::string_view::npos)
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

        context.positional.push_back(arg);
    }
}
