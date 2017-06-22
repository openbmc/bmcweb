#pragma once

/*This file overrides the default crow logging framework to use g3 instead.
 It implements enough of the interfaces of the crow logging framework to work
 correctly but deletes the ILogHandler interface, as usage of that would be
 counter to the g3 handler management, and would cause performance issues.
*/
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
  static void setLogLevel(LogLevel level) {}

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
#ifndef CROW_DISABLE_LOGGING
#define CROW_DISABLE_LOGGING false
#endif

#define CROW_LOG_CRITICAL \
  if (!CROW_DISABLE_LOGGING) LOG(FATAL)
#define CROW_LOG_ERROR \
  if (!CROW_DISABLE_LOGGING) std::cerr
#define CROW_LOG_WARNING \
  if (!CROW_DISABLE_LOGGING) std::cerr
#define CROW_LOG_INFO \
  if (!CROW_DISABLE_LOGGING) LOG(INFO)
#define CROW_LOG_DEBUG \
  if (!CROW_DISABLE_LOGGING) std::cout
