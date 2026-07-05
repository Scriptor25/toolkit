#include <args/args.hxx>

#include <toolkit/string.hxx>

args::manifest::manifest(const std::initializer_list<entry> entries)
    : entries_(entries)
{
    for (const auto &e : entries_)
        for (auto pattern : e.patterns)
        {
            if (auto it = lookup_.find(pattern); it != lookup_.end())
                throw std::runtime_error(
                    std::format(
                        "warning: attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                        pattern,
                        it->second->id,
                        e.id));

            lookup_[pattern] = &e;
        }
}

toolkit::result<> args::manifest::insert(entry e)
{
    for (const auto eit = entries_.insert(entries_.end(), std::move(e));
         auto pattern : eit->patterns)
    {
        if (auto it = lookup_.find(pattern); it != lookup_.end())
            return toolkit::make_error(
                "warning: attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                pattern,
                it->second->id,
                eit->id);

        lookup_[pattern] = &*eit;
    }

    return {};
}

toolkit::result<> args::manifest::insert(std::span<const entry> e)
{
    for (auto eit = entries_.insert(entries_.end(), e.begin(), e.end());
         eit != entries_.end();
         ++eit
    )
        for (auto pattern : eit->patterns)
        {
            if (auto it = lookup_.find(pattern); it != lookup_.end())
                return toolkit::make_error(
                    "warning: attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                    pattern,
                    it->second->id,
                    eit->id);

            lookup_[pattern] = &*eit;
        }

    return {};
}

const args::entry *args::manifest::find(const std::string_view pattern) const
{
    if (const auto it = lookup_.find(pattern); it != lookup_.end())
        return it->second;
    return nullptr;
}

toolkit::result<> args::context::parse_argument(
    context &ctx,
    const entry &e,
    const std::string_view key,
    const std::string_view val)
{
    auto &dst = ctx.values_[e.id];

    if (e.kind == entry_kind::array)
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

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const int argc,
    const char *const *argv)
{
    return parse(man, std::span(argv, argv + argc));
}

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const std::span<const char * const> args)
{
    std::vector<std::string_view> values(args.size());
    for (size_t i = 0; i < args.size(); ++i)
        values[i] = args[i];
    return parse(man, values);
}

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const std::span<const std::string_view> args)
{
    context ctx;

    if (!args.empty())
        ctx.file_ = args[0];

    ctx.limit_ = ~size_t();

    ctx.positional_.clear();
    ctx.flags_.clear();
    ctx.values_.clear();

    auto positional = false;

    for (size_t i = 1; i < args.size(); ++i)
    {
        auto arg = args[i];

        if (positional)
        {
            ctx.positional_.push_back(arg);
            continue;
        }

        if (arg == "--")
        {
            ctx.limit_ = ctx.positional_.size();
            positional = true;
            continue;
        }

        if (auto *e = man.find(arg))
        {
            if (e->kind == entry_kind::flag)
            {
                ctx.flags_.insert(e->id);
                continue;
            }

            if (++i >= args.size())
                return toolkit::make_error(
                    "missing value for argument '{}': reached end of arguments.",
                    arg);

            auto val = args[i];

            if (auto res = parse_argument(ctx, *e, arg, val); !res)
                return res;

            continue;
        }

        if (const auto pos = arg.find('='); pos != std::string_view::npos)
        {
            auto key = arg.substr(0, pos);

            if (auto *e = man.find(key))
            {
                if (e->kind == entry_kind::flag)
                    return toolkit::make_error(
                        "invalid argument kind flag for '{}': must be either value or array.",
                        arg);

                auto val = arg.substr(pos + 1);

                if (auto res = parse_argument(ctx, *e, key, val); !res)
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

                auto *e = man.find(flag);
                if (!e || e->kind != entry_kind::flag)
                {
                    all_flags = false;
                    break;
                }

                flags.insert(e->id);
            }

            if (all_flags)
            {
                ctx.flags_.insert(flags.begin(), flags.end());
                continue;
            }
        }

        ctx.positional_.push_back(arg);
    }

    return ctx;
}

std::string_view args::context::file() const
{
    return file_;
}

bool args::context::limited() const
{
    return limit_ != ~size_t();
}

size_t args::context::limit() const
{
    return limit_;
}

bool args::context::empty() const
{
    return positional_.empty();
}

size_t args::context::size() const
{
    return positional_.size();
}

const std::string_view &args::context::operator[](const size_t index) const
{
    return positional_[index];
}

std::vector<std::string_view>::const_iterator args::context::begin() const
{
    return positional_.begin();
}

std::vector<std::string_view>::const_iterator args::context::end() const
{
    return positional_.end();
}

bool args::context::is(const std::string_view key) const
{
    return flags_.contains(key);
}

std::optional<std::string_view> args::context::get(const std::string_view key) const
{
    if (const auto it = values_.find(key); it != values_.end())
        return it->second.front();

    return std::nullopt;
}

std::span<const std::string_view> args::context::get_all(const std::string_view key) const
{
    if (const auto it = values_.find(key); it != values_.end())
        return it->second;

    return {};
}
