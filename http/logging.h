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
           [[maybe_unused]] const size_t line, LogLevel levelIn) :
        level(levelIn)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        stringstream << "(" << timestamp() << ") [" << prefix << " "
                     << std::filesystem::path(filename).filename() << ":"
                     << line << "] ";
#endif
    }
    ~Logger()
    {
        if (level >= getCurrentLogLevel())
        {
#ifdef BMCWEB_ENABLE_LOGGING
            stringstream << std::endl;
            std::cerr << stringstream.str();
#endif
        }
    }

    //
    template <typename T>
    Logger& operator<<([[maybe_unused]] T const& value)
    {
        if (level >= getCurrentLogLevel())
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

    static LogLevel getCurrentLogLevel()
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
    if (crow::Logger::getCurrentLogLevel() <= crow::LogLevel::Critical)        \
    crow::Logger("CRITICAL", __FILE__, __LINE__, crow::LogLevel::Critical)
#define BMCWEB_LOG_ERROR                                                       \
    if (crow::Logger::getCurrentLogLevel() <= crow::LogLevel::Error)           \
    crow::Logger("ERROR", __FILE__, __LINE__, crow::LogLevel::Error)
#define BMCWEB_LOG_WARNING                                                     \
    if (crow::Logger::getCurrentLogLevel() <= crow::LogLevel::Warning)         \
    crow::Logger("WARNING", __FILE__, __LINE__, crow::LogLevel::Warning)
#define BMCWEB_LOG_INFO                                                        \
    if (crow::Logger::getCurrentLogLevel() <= crow::LogLevel::Info)            \
    crow::Logger("INFO", __FILE__, __LINE__, crow::LogLevel::Info)
#define BMCWEB_LOG_DEBUG                                                       \
    if (crow::Logger::getCurrentLogLevel() <= crow::LogLevel::Debug)           \
    crow::Logger("DEBUG", __FILE__, __LINE__, crow::LogLevel::Debug)
