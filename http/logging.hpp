#pragma once

#include "bmcweb_config.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace crow
{
enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical,
};

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
    constexpr static bool isLoggingEnabled()
    {
#ifdef BMCWEB_ENABLE_LOGGING
        return true;
#else
        return false;
#endif
    }

    Logger([[maybe_unused]] const std::string& prefix,
           [[maybe_unused]] const std::string& filename,
           [[maybe_unused]] const size_t line, LogLevel levelIn) :
        level(levelIn)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        if (crow::Logger::checkLoggingLevel(level))
        {
            stringstream << "(" << timestamp() << ") [" << prefix << " "
                         << std::filesystem::path(filename).filename() << ":"
                         << line << "] ";
        }
#endif
    }
    ~Logger()
    {
#ifdef BMCWEB_ENABLE_LOGGING
        if (crow::Logger::checkLoggingLevel(level))
        {
            stringstream << std::endl;
            std::cerr << stringstream.str();
        }
#endif
    }

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(const Logger&&) = delete;

    //
    template <typename T>
    Logger& operator<<([[maybe_unused]] T const& value)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        if (crow::Logger::checkLoggingLevel(level))
        {
            // Somewhere in the code we're implicitly casting an array to a
            // pointer in logging code. It's non-trivial to find,
            // so disable the check here for now
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            stringstream << value;
        }
#endif
        return *this;
    }

    //
    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel getCurrentLogLevel()
    {
        return getLogLevelRef();
    }

    static bool checkLoggingLevel(const LogLevel level)
    {
        return getCurrentLogLevel() <= level;
    }

  private:
    //
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = static_cast<LogLevel>(bmcwebLoggingLevel);
        return currentLevel;
    }

    //
    std::ostringstream stringstream;
    LogLevel level;
};
} // namespace crow

// The logging functions currently use macros.  Now that we have c++20, ideally
// they'd use source_location with fixed functions, but for the moment, disable
// the check.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_CRITICAL                                                    \
    if constexpr (crow::Logger::isLoggingEnabled())                            \
        if (crow::Logger::checkLoggingLevel(crow::LogLevel::Critical))         \
    crow::Logger("CRITICAL", __FILE__, __LINE__, crow::LogLevel::Critical)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_ERROR                                                       \
    if constexpr (crow::Logger::isLoggingEnabled())                            \
        if (crow::Logger::checkLoggingLevel(crow::LogLevel::Error))            \
    crow::Logger("ERROR", __FILE__, __LINE__, crow::LogLevel::Error)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_WARNING                                                     \
    if constexpr (crow::Logger::isLoggingEnabled())                            \
        if (crow::Logger::checkLoggingLevel(crow::LogLevel::Warning))          \
    crow::Logger("WARNING", __FILE__, __LINE__, crow::LogLevel::Warning)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_INFO                                                        \
    if constexpr (crow::Logger::isLoggingEnabled())                            \
        if (crow::Logger::checkLoggingLevel(crow::LogLevel::Info))             \
    crow::Logger("INFO", __FILE__, __LINE__, crow::LogLevel::Info)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BMCWEB_LOG_DEBUG                                                       \
    if constexpr (crow::Logger::isLoggingEnabled())                            \
        if (crow::Logger::checkLoggingLevel(crow::LogLevel::Debug))            \
    crow::Logger("DEBUG", __FILE__, __LINE__, crow::LogLevel::Debug)
