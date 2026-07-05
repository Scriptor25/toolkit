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

toolkit::result<toolkit::arg_context> toolkit::arg_parse(
    const arg_manifest &manifest,
    std::span<const std::string_view> args)
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

const std::unordered_map<std::errc, const char *> toolkit::error_strings_map
{
    { std::errc::address_family_not_supported, "address_family_not_supported" },
    { std::errc::address_in_use, "address_in_use" },
    { std::errc::address_not_available, "address_not_available" },
    { std::errc::already_connected, "already_connected" },
    { std::errc::argument_list_too_long, "argument_list_too_long" },
    { std::errc::argument_out_of_domain, "argument_out_of_domain" },
    { std::errc::bad_address, "bad_address" },
    { std::errc::bad_file_descriptor, "bad_file_descriptor" },
    { std::errc::bad_message, "bad_message" },
    { std::errc::broken_pipe, "broken_pipe" },
    { std::errc::connection_aborted, "connection_aborted" },
    { std::errc::connection_already_in_progress, "connection_already_in_progress" },
    { std::errc::connection_refused, "connection_refused" },
    { std::errc::connection_reset, "connection_reset" },
    { std::errc::cross_device_link, "cross_device_link" },
    { std::errc::destination_address_required, "destination_address_required" },
    { std::errc::device_or_resource_busy, "device_or_resource_busy" },
    { std::errc::directory_not_empty, "directory_not_empty" },
    { std::errc::executable_format_error, "executable_format_error" },
    { std::errc::file_exists, "file_exists" },
    { std::errc::file_too_large, "file_too_large" },
    { std::errc::filename_too_long, "filename_too_long" },
    { std::errc::function_not_supported, "function_not_supported" },
    { std::errc::host_unreachable, "host_unreachable" },
    { std::errc::identifier_removed, "identifier_removed" },
    { std::errc::illegal_byte_sequence, "illegal_byte_sequence" },
    { std::errc::inappropriate_io_control_operation, "inappropriate_io_control_operation" },
    { std::errc::interrupted, "interrupted" },
    { std::errc::invalid_argument, "invalid_argument" },
    { std::errc::invalid_seek, "invalid_seek" },
    { std::errc::io_error, "io_error" },
    { std::errc::is_a_directory, "is_a_directory" },
    { std::errc::message_size, "message_size" },
    { std::errc::network_down, "network_down" },
    { std::errc::network_reset, "network_reset" },
    { std::errc::network_unreachable, "network_unreachable" },
    { std::errc::no_buffer_space, "no_buffer_space" },
    { std::errc::no_child_process, "no_child_process" },
    { std::errc::no_link, "no_link" },
    { std::errc::no_lock_available, "no_lock_available" },
    { std::errc::no_message_available, "no_message_available" },
    { std::errc::no_message, "no_message" },
    { std::errc::no_protocol_option, "no_protocol_option" },
    { std::errc::no_space_on_device, "no_space_on_device" },
    { std::errc::no_stream_resources, "no_stream_resources" },
    { std::errc::no_such_device_or_address, "no_such_device_or_address" },
    { std::errc::no_such_device, "no_such_device" },
    { std::errc::no_such_file_or_directory, "no_such_file_or_directory" },
    { std::errc::no_such_process, "no_such_process" },
    { std::errc::not_a_directory, "not_a_directory" },
    { std::errc::not_a_socket, "not_a_socket" },
    { std::errc::not_a_stream, "not_a_stream" },
    { std::errc::not_connected, "not_connected" },
    { std::errc::not_enough_memory, "not_enough_memory" },
    { std::errc::not_supported, "not_supported" },
    { std::errc::operation_canceled, "operation_canceled" },
    { std::errc::operation_in_progress, "operation_in_progress" },
    { std::errc::operation_not_permitted, "operation_not_permitted" },
    { std::errc::operation_not_supported, "operation_not_supported" },
    { std::errc::operation_would_block, "operation_would_block" },
    { std::errc::owner_dead, "owner_dead" },
    { std::errc::permission_denied, "permission_denied" },
    { std::errc::protocol_error, "protocol_error" },
    { std::errc::protocol_not_supported, "protocol_not_supported" },
    { std::errc::read_only_file_system, "read_only_file_system" },
    { std::errc::resource_deadlock_would_occur, "resource_deadlock_would_occur" },
    { std::errc::resource_unavailable_try_again, "resource_unavailable_try_again" },
    { std::errc::result_out_of_range, "result_out_of_range" },
    { std::errc::state_not_recoverable, "state_not_recoverable" },
    { std::errc::stream_timeout, "stream_timeout" },
    { std::errc::text_file_busy, "text_file_busy" },
    { std::errc::timed_out, "timed_out" },
    { std::errc::too_many_files_open_in_system, "too_many_files_open_in_system" },
    { std::errc::too_many_files_open, "too_many_files_open" },
    { std::errc::too_many_links, "too_many_links" },
    { std::errc::too_many_symbolic_link_levels, "too_many_symbolic_link_levels" },
    { std::errc::value_too_large, "value_too_large" },
    { std::errc::value_too_large, "value_too_large" },
    { std::errc::wrong_protocol_type, "wrong_protocol_type" },
};
