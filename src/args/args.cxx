#include <args/args.hxx>

#include <toolkit/string.hxx>

#include <algorithm>

toolkit::result<args::manifest> args::manifest::from(std::initializer_list<entry> entries)
{
    manifest man;
    if (auto res = man.insert(entries); !res)
        return res;
    return man;
}

toolkit::result<> args::manifest::insert(entry e)
{
    if (e.id.empty())
        return toolkit::make_error("empty entry id.");
    if (e.patterns.empty())
        return toolkit::make_error("empty entry patterns.");

    if (ids_.contains(e.id))
        return toolkit::make_error("duplicate entry id '{}'.", e.id);

    for (const auto &p : e.patterns)
    {
        if (p.empty())
            return toolkit::make_error("empty entry pattern.");

        if (auto it = lookup_.find(p); it != lookup_.end())
            return toolkit::make_error(
                "attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                p,
                entries_[it->second].id,
                e.id);
    }

    const auto index = entries_.size();
    entries_.push_back(std::move(e));

    ids_.insert(entries_[index].id);

    for (const auto &p : entries_[index].patterns)
        lookup_.emplace(p, index);

    return {};
}

toolkit::result<> args::manifest::insert(std::span<const entry> es)
{
    std::unordered_set<std::string> ids;
    std::unordered_map<std::string, std::string> lookup;

    for (const auto &e : es)
    {
        if (e.id.empty())
            return toolkit::make_error("empty entry id.");
        if (e.patterns.empty())
            return toolkit::make_error("empty entry patterns.");

        if (ids_.contains(e.id) || ids.contains(e.id))
            return toolkit::make_error("duplicate entry id '{}'.", e.id);

        ids.insert(e.id);

        for (const auto &p : e.patterns)
        {
            if (p.empty())
                return toolkit::make_error("empty entry pattern.");

            if (auto it = lookup_.find(p); it != lookup_.end())
                return toolkit::make_error(
                    "attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                    p,
                    entries_[it->second].id,
                    e.id);
            if (auto it = lookup.find(p); it != lookup.end())
                return toolkit::make_error(
                    "attempt to override pattern '{}': first defined by '{}', then by '{}'.",
                    p,
                    it->second,
                    e.id);

            lookup.emplace(p, e.id);
        }
    }

    const auto first = entries_.size();
    entries_.insert(entries_.end(), es.begin(), es.end());

    for (auto index = first; index < entries_.size(); ++index)
    {
        ids_.insert(entries_[index].id);

        for (const auto &p : entries_[index].patterns)
            lookup_.emplace(p, index);
    }

    return {};
}

const args::entry *args::manifest::find(const std::string &pattern) const
{
    if (const auto it = lookup_.find(pattern); it != lookup_.end())
        return &entries_[it->second];
    return nullptr;
}

void args::manifest::describe(std::ostream &stream) const
{
    std::size_t width{};
    std::vector<std::string> lines;
    lines.reserve(entries_.size());

    for (const auto &e : entries_)
    {
        std::vector ps(e.patterns.begin(), e.patterns.end());
        std::ranges::sort(ps);

        auto &line = lines.emplace_back();
        for (auto it = ps.begin(); it != ps.end(); ++it)
        {
            if (it != ps.begin())
                line += ", ";
            line += *it;
        }

        width = std::max(width, line.size());
    }

    for (std::size_t i{}; i < lines.size(); ++i)
    {
        const auto &e = entries_[i];
        const auto &line = lines[i];

        const auto rem = width - line.size();
        const std::string spacing(rem + 2, ' ');

        stream << line << spacing << e.description << "\r\n";
    }

    stream.flush();
}

toolkit::result<> args::context::parse_argument(
    context &ctx,
    const entry &e,
    const std::string &key,
    const std::string &val)
{
    auto &dst = ctx.values_[e.id];

    if (e.kind == entry_kind::array)
    {
        std::vector<std::string> vals;
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

bool args::context::parse_flags(
    const manifest &man,
    const std::string &arg,
    context *ctx)
{
    if (!arg.starts_with('-') || arg.starts_with("--") || arg.size() <= 2)
        return false;

    for (std::size_t i = 1; i < arg.size(); ++i)
    {
        std::string flag = "-";
        flag += arg[i];

        if (const auto *e = man.find(flag); !e || e->kind != entry_kind::flag)
            return false;
    }

    if (ctx)
        for (std::size_t i = 1; i < arg.size(); ++i)
        {
            std::string flag = "-";
            flag += arg[i];

            const auto *e = man.find(flag);
            ++ctx->flags_[e->id];
        }

    return true;
}

bool args::context::is_option(
    const manifest &man,
    const std::string &arg)
{
    if (man.find(arg))
        return true;

    if (parse_flags(man, arg))
        return true;

    if (const auto pos = arg.find('='); pos != std::string::npos)
        return man.find(arg.substr(0, pos)) != nullptr;

    return false;
}

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const int argc,
    const char *const *argv)
{
    if (argc < 0 || (argc > 0 && !argv))
        return toolkit::make_error("invalid argument vector.");

    if (!argc)
        return parse(man, std::span<const char *const>());

    return parse(man, std::span(argv, static_cast<std::size_t>(argc)));
}

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const std::span<const char * const> args)
{
    std::vector<std::string> values(args.size());
    for (std::size_t i{}; i < args.size(); ++i)
    {
        if (!args[i])
            return toolkit::make_error("null argument at index {}.", i);

        values[i] = args[i];
    }
    return parse(man, values);
}

toolkit::result<args::context> args::context::parse(
    const manifest &man,
    const std::span<const std::string> args)
{
    context ctx;

    if (!args.empty())
        ctx.file_ = args[0];

    ctx.limit_ = ~std::size_t();

    ctx.positional_.clear();
    ctx.flags_.clear();
    ctx.values_.clear();

    auto positional = false;

    for (std::size_t i = 1; i < args.size(); ++i)
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
                ++ctx.flags_[e->id];
                continue;
            }

            if (++i >= args.size())
                return toolkit::make_error(
                    "missing value for argument '{}': reached end of arguments.",
                    arg);

            const auto &val = args[i];

            if (val == "--" || is_option(man, val))
                return toolkit::make_error("missing value for argument '{}': encountered another option.", arg);

            if (auto res = parse_argument(ctx, *e, arg, val); !res)
                return res;

            continue;
        }

        if (const auto pos = arg.find('='); pos != std::string::npos)
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

        if (parse_flags(man, arg, &ctx))
            continue;

        if (arg.starts_with('-'))
            return toolkit::make_error("unknown argument '{}'.", arg);

        ctx.positional_.push_back(arg);
    }

    return ctx;
}

std::string args::context::file() const
{
    return file_;
}

bool args::context::limited() const
{
    return limit_ != ~std::size_t();
}

std::size_t args::context::limit() const
{
    return limit_;
}

bool args::context::empty() const
{
    return positional_.empty();
}

std::size_t args::context::size() const
{
    return positional_.size();
}

const std::string &args::context::operator[](const std::size_t index) const
{
    return positional_[index];
}

std::vector<std::string>::const_iterator args::context::begin() const
{
    return positional_.begin();
}

std::vector<std::string>::const_iterator args::context::end() const
{
    return positional_.end();
}

bool args::context::is(const std::string &key) const
{
    return count(key) != std::size_t{};
}

std::size_t args::context::count(const std::string &key) const
{
    if (const auto it = flags_.find(key); it != flags_.end())
        return it->second;
    return {};
}

std::optional<std::string> args::context::get(const std::string &key) const
{
    if (const auto it = values_.find(key); it != values_.end() && !it->second.empty())
        return it->second.front();

    return std::nullopt;
}

std::span<const std::string> args::context::get_all(const std::string &key) const
{
    if (const auto it = values_.find(key); it != values_.end())
        return it->second;

    return {};
}
