#pragma once

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

class logger
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
    logger(const std::string& prefix, const std::string& filename,
           const size_t line, LogLevel levelIn) :
        level(levelIn)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        stringstream << "(" << timestamp() << ") [" << prefix << " "
                     << std::filesystem::path(filename).filename() << ":"
                     << line << "] ";
#endif
    }
    ~logger()
    {
        if (level >= get_current_log_level())
        {
#ifdef BMCWEB_ENABLE_LOGGING
            stringstream << std::endl;
            std::cerr << stringstream.str();
#endif
        }
    }

    //
    template <typename T> logger& operator<<(T const& value)
    {
        if (level >= get_current_log_level())
        {
#ifdef BMCWEB_ENABLE_LOGGING
            stringstream << value;
#endif
        }
        return *this;
    }

    //
    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel get_current_log_level()
    {
        return getLogLevelRef();
    }

  private:
    //
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = static_cast<LogLevel>(1);
        return currentLevel;
    }

    //
    std::ostringstream stringstream;
    LogLevel level;
};
} // namespace crow

#define BMCWEB_LOG_CRITICAL                                                    \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Critical)     \
    crow::logger("CRITICAL", __FILE__, __LINE__, crow::LogLevel::Critical)
#define BMCWEB_LOG_ERROR                                                       \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Error)        \
    crow::logger("ERROR", __FILE__, __LINE__, crow::LogLevel::Error)
#define BMCWEB_LOG_WARNING                                                     \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Warning)      \
    crow::logger("WARNING", __FILE__, __LINE__, crow::LogLevel::Warning)
#define BMCWEB_LOG_INFO                                                        \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Info)         \
    crow::logger("INFO", __FILE__, __LINE__, crow::LogLevel::Info)
#define BMCWEB_LOG_DEBUG                                                       \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Debug)        \
    crow::logger("DEBUG", __FILE__, __LINE__, crow::LogLevel::Debug)
