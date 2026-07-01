#include <toolkit/args.hxx>
#include <toolkit/string.hxx>

#include <iostream>

toolkit::arg_manifest::arg_manifest(std::vector<arg_entry> arg_entries)
    : entries(std::move(arg_entries))
{
    for (const auto &entry : entries)
        for (auto pattern : entry.patterns)
        {
            if (auto it = lookup.find(pattern); it != lookup.end())
                throw std::runtime_error(
                    std::format(
                        "warning: attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                        pattern,
                        it->second->id,
                        entry.id));

            lookup[pattern] = &entry;
        }
}

void toolkit::arg_manifest::push_back(arg_entry &&entry)
{
    entries.push_back(std::move(entry));

    for (auto &e = entries.back(); auto pattern : e.patterns)
    {
        if (auto it = lookup.find(pattern); it != lookup.end())
            throw std::runtime_error(
                std::format(
                    "warning: attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                    pattern,
                    it->second->id,
                    e.id));

        lookup[pattern] = &e;
    }
}

const toolkit::arg_entry *toolkit::arg_manifest::find(const std::string_view pattern) const
{
    if (const auto it = lookup.find(pattern); it != lookup.end())
        return it->second;
    return nullptr;
}

bool toolkit::arg_context::is(const std::string_view key) const
{
    return flags.contains(key);
}

std::optional<std::string_view> toolkit::arg_context::get(const std::string_view key) const
{
    if (const auto it = values.find(key); it != values.end())
        return it->second.front();

    return std::nullopt;
}

std::span<const std::string_view> toolkit::arg_context::get_all(const std::string_view key) const
{
    if (const auto it = values.find(key); it != values.end())
        return it->second;

    return {};
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

static toolkit::result<> parse_argument(
    toolkit::arg_context &context,
    const toolkit::arg_entry &entry,
    const std::string_view key,
    const std::string_view val)
{
    auto &dst = context.values[entry.id];

    if (entry.kind == toolkit::arg_kind::array)
    {
        std::vector<std::string_view> vals;
        toolkit::split(vals, val, ',');

        dst.insert(dst.end(), vals.begin(), vals.end());
        return {};
    }

    if (dst.empty())
    {
        dst.push_back(val);
        return {};
    }

    return toolkit::make_error("duplicate argument '{}'.", key);
}

toolkit::result<toolkit::arg_context> toolkit::arg_parse(
    const arg_manifest &manifest,
    const int argc,
    const char *const *argv)
{
    return arg_parse(manifest, std::span(argv, argv + argc));
}

toolkit::result<toolkit::arg_context> toolkit::arg_parse(
    const arg_manifest &manifest,
    const std::span<const char * const> args)
{
    std::vector<std::string_view> values(args.size());
    for (size_t i = 0; i < args.size(); ++i)
        values[i] = args[i];
    return arg_parse(manifest, values);
}

toolkit::result<toolkit::arg_context> toolkit::arg_parse(const arg_manifest &manifest, std::span<std::string_view> args)
{
    arg_context context;

    if (!args.empty())
        context.file = args[0];

    context.limit = ~size_t();

    context.positional.clear();
    context.flags.clear();
    context.values.clear();

    auto positional = false;

    for (size_t i = 1; i < args.size(); ++i)
    {
        auto arg = args[i];

        if (positional)
        {
            context.positional.push_back(arg);
            continue;
        }

        if (arg == "--")
        {
            context.limit = context.positional.size();
            positional = true;
            continue;
        }

        if (auto *entry = manifest.find(arg))
        {
            if (entry->kind == arg_kind::flag)
            {
                context.flags.insert(entry->id);
                continue;
            }

            if (++i >= args.size())
                return make_error("missing value for argument '{}': reached end of arguments.", arg);

            auto val = args[i];

            if (auto res = parse_argument(context, *entry, arg, val); !res)
                return res;

            continue;
        }

        if (const auto pos = arg.find('='); pos != std::string_view::npos)
        {
            auto key = arg.substr(0, pos);

            if (auto *entry = manifest.find(key))
            {
                if (entry->kind == arg_kind::flag)
                    return make_error("invalid argument kind flag for '{}': must be either value or array.", arg);

                auto val = arg.substr(pos + 1);

                if (auto res = parse_argument(context, *entry, key, val); !res)
                    return res;

                continue;
            }
        }

        if (arg.starts_with("-") && !arg.starts_with("--") && arg.size() > 2)
        {
            auto all_flags = true;
            std::unordered_set<std::string_view> flags;

            for (size_t j = 1; j < arg.size(); ++j)
            {
                std::string flag = "-";
                flag += arg[j];

                auto *entry = manifest.find(flag);
                if (!entry || entry->kind != arg_kind::flag)
                {
                    all_flags = false;
                    break;
                }

                flags.insert(entry->id);
            }

            if (all_flags)
            {
                context.flags.insert(flags.begin(), flags.end());
                continue;
            }
        }

        context.positional.push_back(arg);
    }

    return context;
}

std::istream &toolkit::get_line(std::istream &stream, std::string &string, const std::string_view delimiter)
{
    string.clear();

    while (stream.good() && !stream.eof() && string.find(delimiter) == std::string::npos)
    {
        const auto c = stream.get();
        if (c < 0)
        {
            break;
        }

        string += static_cast<char>(c);
    }

    if (string.find(delimiter) == std::string::npos)
    {
        return stream;
    }

    string = string.substr(0, string.size() - delimiter.size());
    return stream;
}
