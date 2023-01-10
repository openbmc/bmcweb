#pragma once

#include "bmcweb_config.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace crow
{
enum class LogLevel
{
    Disabled = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

// Mapping of the external loglvl name to internal loglvl
constexpr std::array<std::pair<std::string_view, crow::LogLevel>, 7>
    mapLogLevelFromName{{{"disabled", crow::LogLevel::Disabled},
                         {"enabled", crow::LogLevel::Debug},
                         {"debug", crow::LogLevel::Debug},
                         {"info", crow::LogLevel::Info},
                         {"warning", crow::LogLevel::Warning},
                         {"error", crow::LogLevel::Error},
                         {"critical", crow::LogLevel::Critical}}};

constexpr crow::LogLevel getLogLevelFromName(std::string_view name)
{
    const auto* iter =
        std::find_if(begin(mapLogLevelFromName), end(mapLogLevelFromName),
                     [&name](const auto& v) { return v.first == name; });
    if (iter != end(mapLogLevelFromName))
    {
        return iter->second;
    }
    return crow::LogLevel::Disabled;
}

// configured bmcweb LogLevel
constexpr crow::LogLevel bmcwebCurrentLoggingLevel =
    getLogLevelFromName(bmcwebLoggingLevel);

class Logger
{
  private:
    //
    static std::string timestamp()
    {
        std::string date;
        date.resize(32, '\0');
        time_t t = time(nullptr);

        tm myTm{};

        gmtime_r(&t, &myTm);

        size_t sz =
            strftime(date.data(), date.size(), "%Y-%m-%d %H:%M:%S", &myTm);
        date.resize(sz);
        return date;
    }

  public:
    Logger([[maybe_unused]] const std::string& prefix,
           [[maybe_unused]] const std::string& filename,
           [[maybe_unused]] const size_t line)
    {
        stringstream << "(" << timestamp() << ") [" << prefix << " "
                     << std::filesystem::path(filename).filename() << ":"
                     << line << "] ";
    }
    ~Logger()
    {
        stringstream << std::endl;
        std::cerr << stringstream.str();
    }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(const Logger&&) = delete;

    //
    template <typename T>
    Logger& operator<<([[maybe_unused]] T const& value)
    {
        // Somewhere in the code we're implicitly casting an array to a
        // pointer in logging code. It's non-trivial to find,
        // so disable the check here for now
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        stringstream << value;
        return *this;
    }

    constexpr static LogLevel getCurrentLogLevel()
    {
        return bmcwebCurrentLoggingLevel;
    }

    constexpr static bool isLoggingEnabled()
    {
        return getCurrentLogLevel() >= crow::LogLevel::Debug;
    }

    constexpr static bool checkLoggingLevel(const LogLevel level)
    {
        return isLoggingEnabled() && (getCurrentLogLevel() <= level);
    }

  private:
    //
    std::ostringstream stringstream;
};
} // namespace crow

// The logging functions currently use macros.  Now that we have c++20, ideally
// they'd use source_location with fixed functions, but for the moment, disable
// the check.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_CRITICAL                                                    \
    if constexpr (crow::Logger::checkLoggingLevel(crow::LogLevel::Critical))   \
    crow::Logger("CRITICAL", __FILE__, __LINE__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_ERROR                                                       \
    if constexpr (crow::Logger::checkLoggingLevel(crow::LogLevel::Error))      \
    crow::Logger("ERROR", __FILE__, __LINE__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_WARNING                                                     \
    if constexpr (crow::Logger::checkLoggingLevel(crow::LogLevel::Warning))    \
    crow::Logger("WARNING", __FILE__, __LINE__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_INFO                                                        \
    if constexpr (crow::Logger::checkLoggingLevel(crow::LogLevel::Info))       \
    crow::Logger("INFO", __FILE__, __LINE__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_DEBUG                                                       \
    if constexpr (crow::Logger::checkLoggingLevel(crow::LogLevel::Debug))      \
    crow::Logger("DEBUG", __FILE__, __LINE__)
