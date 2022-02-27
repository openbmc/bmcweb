#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

#include <boost/system/error_code.hpp>
#include <source_location.hpp>

#include <system_error>

enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical,

    Max,
};

constexpr std::array<std::string_view, static_cast<size_t>(LogLevel::Max)>
    levelStrings{"Debug", "Info", "Warning", "Error", "Critical"};

static constexpr LogLevel currentLevel = LogLevel::Debug;

struct FormatString
{
    fmt::string_view str;
    bmcweb::source_location loc;

    FormatString(const char* str, const bmcweb::source_location& loc =
                                      bmcweb::source_location::current()) :
        str(str),
        loc(loc)
    {}
};

template <LogLevel level>
inline void vlog(const FormatString& format, fmt::format_args args)
{
    if (currentLevel <= level)
    {
        const auto& loc = format.loc;
        constexpr size_t stringIndex = static_cast<size_t>(level);
        static_assert(stringIndex < levelStrings.size(),
                      "Missing string for level");
        std::string_view levelString = levelStrings[stringIndex];
        std::string_view filename = loc.file_name();
        if (filename.starts_with("../"))
        {
            filename = filename.substr(3);
        }
        fmt::print("[{} {}:{}] ", levelString, filename, loc.line());
        fmt::vprint(format.str, args);
        std::putc('\n', stdout);
    }
}

template <typename... Args>
inline void BMCWEB_LOG_CRITICAL(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Critical>(format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void BMCWEB_LOG_ERROR(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Error>(format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void BMCWEB_LOG_WARNING(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Warning>(format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void BMCWEB_LOG_INFO(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Info>(format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void BMCWEB_LOG_DEBUG(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Debug>(format, fmt::make_format_args(args...));
}

template <>
struct fmt::formatter<boost::system::error_code>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end)
        {
            it++;
        }
        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }
    template <typename FormatContext>
    auto format(const boost::system::error_code& ec, FormatContext& ctx)
        -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", ec.message());
    }
};

template <>
struct fmt::formatter<std::error_code>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end)
        {
            it++;
        }
        if (it != end && *it != '}')
        {
            throw format_error("invalid format");
        }

        return it;
    }
    template <typename FormatContext>
    auto format(const std::error_code& ec, FormatContext& ctx)
        -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", ec.message());
    }
};
