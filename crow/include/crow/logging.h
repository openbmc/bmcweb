#pragma once

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

namespace crow
{
enum class LogLevel
{
#ifndef ERROR
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
#endif

    Debug = 0,
    Info,
    Warning,
    Error,
    Critical,
};

class ILogHandler
{
  public:
    virtual void log(std::string message, LogLevel level) = 0;
};

class CerrLogHandler : public ILogHandler
{
  public:
    void log(std::string message, LogLevel /*level*/) override
    {
        std::cerr << message;
    }
};

class logger
{
  private:
    //
    static std::string timestamp()
    {
        char date[32];
        time_t t = time(0);

        tm myTm{};

#ifdef _MSC_VER
        gmtime_s(&my_tm, &t);
#else
        gmtime_r(&t, &myTm);
#endif

        size_t sz = strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &myTm);
        return std::string(date, date + sz);
    }

  public:
    logger(const std::string& prefix, LogLevel level) : level(level)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        stringstream << "(" << timestamp() << ") [" << prefix << "] ";
#endif
    }
    ~logger()
    {
#ifdef BMCWEB_ENABLE_LOGGING
        if (level >= get_current_log_level())
        {
            stringstream << std::endl;
            getHandlerRef()->log(stringstream.str(), level);
        }
#endif
    }

    //
    template <typename T> logger& operator<<(T const& value)
    {
#ifdef BMCWEB_ENABLE_LOGGING
        if (level >= get_current_log_level())
        {
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

    static void setHandler(ILogHandler* handler)
    {
        getHandlerRef() = handler;
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
    static ILogHandler*& getHandlerRef()
    {
        static CerrLogHandler defaultHandler;
        static ILogHandler* currentHandler = &defaultHandler;
        return currentHandler;
    }

    //
    std::ostringstream stringstream;
    LogLevel level;
};
} // namespace crow

#define BMCWEB_LOG_CRITICAL                                                    \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Critical)     \
    crow::logger("CRITICAL", crow::LogLevel::Critical)
#define BMCWEB_LOG_ERROR                                                       \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Error)        \
    crow::logger("ERROR   ", crow::LogLevel::Error)
#define BMCWEB_LOG_WARNING                                                     \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Warning)      \
    crow::logger("WARNING ", crow::LogLevel::Warning)
#define BMCWEB_LOG_INFO                                                        \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Info)         \
    crow::logger("INFO    ", crow::LogLevel::Info)
#define BMCWEB_LOG_DEBUG                                                       \
    if (crow::logger::get_current_log_level() <= crow::LogLevel::Debug)        \
    crow::logger("DEBUG   ", crow::LogLevel::Debug)
