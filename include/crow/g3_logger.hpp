#pragma once

// This file overrides the default crow logging framework to use g3 instead.
// It implements enough of the interfaces of the crow logging framework to work correctly
// but deletes the ILogHandler interface, as usage of that would be counter to the g3
// handler management, and would cause performance issues.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>

namespace crow {
enum class LogLevel {
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

class logger {
 public:
  logger(std::string prefix, LogLevel level) {
    // no op, let g3 handle th log levels
  }

  //
  template <typename T>
  logger& operator<<(T const& value) {
    return *this;
  }

  //
  static void setLogLevel(LogLevel level) { }

  static LogLevel get_current_log_level() { return get_log_level_ref(); }

 private:
  //
  static LogLevel& get_log_level_ref() {
    static LogLevel current_level = LogLevel::DEBUG;
    return current_level;
  }

  //
  std::ostringstream stringstream_;
  LogLevel level_;
};
}

#define CROW_LOG_CRITICAL LOG(FATAL)
#define CROW_LOG_ERROR LOG(WARNING)
#define CROW_LOG_WARNING LOG(WARNING)
#define CROW_LOG_INFO LOG(INFO)
#define CROW_LOG_DEBUG LOG(DEBUG)



/*
#define CROW_LOG_CRITICAL   \
        if (false) \
            crow::logger("CRITICAL", crow::LogLevel::Critical)
#define CROW_LOG_ERROR      \
        if (false) \
            crow::logger("ERROR   ", crow::LogLevel::Error)
#define CROW_LOG_WARNING    \
        if (false) \
            crow::logger("WARNING ", crow::LogLevel::Warning)
#define CROW_LOG_INFO       \
        if (false) \
            crow::logger("INFO    ", crow::LogLevel::Info)
#define CROW_LOG_DEBUG      \
        if (false) \
            crow::logger("DEBUG   ", crow::LogLevel::Debug)
*/