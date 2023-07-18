#pragma once

#include <boost/system/error_code.hpp>
#include <boost/url/pct_string_view.hpp>
#include <boost/url/string_view.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <bit>
#include <format>
#include <iostream>
#include <source_location>
#include <system_error>

template <>
struct std::formatter<boost::system::error_code>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const boost::system::error_code& ec, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ec.what());
    }
};

template <>
struct std::formatter<std::error_code>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const std::error_code& ec, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ec.message());
    }
};

template <>
struct std::formatter<boost::urls::pct_string_view>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const boost::urls::pct_string_view& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}",
                              std::string_view(msg.data(), msg.size()));
    }
};

template <>
struct std::formatter<boost::urls::url>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const boost::urls::url& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(msg.buffer()));
    }
};

template <>
struct std::formatter<boost::core::string_view>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const boost::core::string_view& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(msg));
    }
};

template <>
struct std::formatter<void*>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const void*& ptr, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}",
                              std::to_string(std::bit_cast<size_t>(ptr)));
    }
};

template <>
struct std::formatter<nlohmann::json::json_pointer>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const nlohmann::json::json_pointer& ptr, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ptr.to_string());
    }
};

template <>
struct std::formatter<nlohmann::json>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const nlohmann::json& ptr, auto& ctx) const
    {
        return std::format_to(
            ctx.out(), "{}",
            ptr.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    }
};

template <>
struct std::formatter<std::filesystem::path>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const std::filesystem::path& ptr, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ptr.string());
    }
};

template <>
struct std::formatter<std::exception>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const std::exception& except, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(except.what()));
    }
};

enum class LogLevel
{
    Disabled = 0,
    Debug,
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
    std::string_view str;
    std::source_location loc;

    // NOLINTNEXTLINE(google-explicit-constructor)
    FormatString(const char* strIn, const std::source_location& locIn =
                                        std::source_location::current()) :
        str(strIn),
        loc(locIn)
    {}
};

template <typename T>
auto logPtr(T p) -> const void*
{
    static_assert(std::is_pointer<T>::value,
                  "Can't use logPtr without pointer");
    return std::bit_cast<const void*>(p);
}

template <LogLevel level>
inline void vlog(const FormatString& format, std::format_args&& args)
{
    if constexpr (currentLevel > level)
    {
        return;
    }
    constexpr size_t stringIndex = static_cast<size_t>(level);
    static_assert(stringIndex < levelStrings.size(),
                  "Missing string for level");
    std::string_view levelString = levelStrings[stringIndex];
    std::string_view filename = format.loc.file_name();
    if (filename.starts_with("../"))
    {
        filename = filename.substr(3);
    }
    std::cout << std::format("[{} {}:{}] ", levelString, filename,
                             format.loc.line());
    std::cout << std::vformat(format.str, args);
    std::putc('\n', stdout);
}

template <typename... Args>
inline void BMCWEB_LOG_CRITICAL(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Critical>(
        format, std::make_format_args(std::forward<Args>(args)...));
}

template <typename... Args>
inline void BMCWEB_LOG_ERROR(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Error>(format,
                          std::make_format_args(std::forward<Args>(args)...));
}

template <typename... Args>
inline void BMCWEB_LOG_WARNING(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Warning>(format,
                            std::make_format_args(std::forward<Args>(args)...));
}

template <typename... Args>
inline void BMCWEB_LOG_INFO(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Info>(format,
                         std::make_format_args(std::forward<Args>(args)...));
}

template <typename... Args>
inline void BMCWEB_LOG_DEBUG(const FormatString& format, Args&&... args)
{
    vlog<LogLevel::Debug>(format,
                          std::make_format_args(std::forward<Args>(args)...));
}
