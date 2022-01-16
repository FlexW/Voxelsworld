#include "log.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace
{
std::string to_string(LogLevel level)
{
  switch (level)
  {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR";
  }
  assert(0);
  return "DEBUG";
}

constexpr auto ansi_off    = "\033[0m";
constexpr auto ansi_red    = "\033[1;31m";
constexpr auto ansi_blue   = "\033[0;34m";
constexpr auto ansi_yellow = "\033[0;33m";
constexpr auto ansi_grey   = "\033[0;90m";
} // namespace

LogLevel Log::level = LogLevel::Debug;

LogLevel Log::reporting_level() { return level; }

void Log::set_reporting_level(LogLevel value) { level = value; }

Log::Log() = default;

Log::~Log()
{
  if (message_level_ >= reporting_level())
  {
    os_ << std::endl;
    if (message_level_ == LogLevel::Error)
    {
      std::cerr << ansi_red << os_.str() << ansi_off;
    }
    else if (message_level_ == LogLevel::Warning)
    {
      std::cerr << ansi_yellow << os_.str() << ansi_off;
    }
    else if (message_level_ == LogLevel::Info)
    {
      std::cerr << ansi_blue << os_.str() << ansi_off;
    }
    else
    {
      std::cerr << ansi_grey << os_.str() << ansi_off;
    }
    std::fflush(stderr);
  }
}

std::ostringstream &Log::get(LogLevel level)
{
  os_ << to_string(level) << " - ";

  message_level_ = level;

  return os_;
}
