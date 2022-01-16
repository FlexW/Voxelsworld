#pragma once

#include <sstream>

enum class LogLevel
{
  Debug   = 0,
  Info    = 1,
  Warning = 2,
  Error   = 3,
};

class Log
{
public:
  static LogLevel reporting_level();
  static void     set_reporting_level(LogLevel value);

  Log();
  ~Log();

  std::ostringstream &get(LogLevel level = LogLevel::Info);

private:
  static LogLevel level;

  LogLevel           message_level_ = LogLevel::Info;
  std::ostringstream os_;

  Log(const Log &) = delete;
  void operator=(const Log &) = delete;
};

#define LOG_DEBUG() Log().get(LogLevel::Debug)
#define LOG_INFO()  Log().get(LogLevel::Info)
#define LOG_WARN()  Log().get(LogLevel::Warning)
#define LOG_ERROR() Log().get(LogLevel::Error)
