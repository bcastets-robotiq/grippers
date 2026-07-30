// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

#include <Robotiq/gripper/logger.hpp>

// StderrLogger is a desktop convenience: it needs <iostream> (which pulls in
// wide-char printf and ~25 KB) and a wall clock (system_clock -> _gettimeofday),
// neither of which exists usefully on a freestanding target. On embedded builds
// (GRIPPERS_BUILD_DEFAULT_SERIAL=0) it degrades to a no-op and makeDefaultLogger
// returns a NullLogger; provide your own Logger (e.g. a UART sink) for real logs.
#ifndef GRIPPERS_BUILD_DEFAULT_SERIAL
#define GRIPPERS_BUILD_DEFAULT_SERIAL 1
#endif

#if GRIPPERS_BUILD_DEFAULT_SERIAL
#include <ctime>
#include <iomanip>
#include <iostream>
#endif

namespace Robotiq {
#if GRIPPERS_BUILD_DEFAULT_SERIAL
namespace {
const char* toString(Logger::Level level)
{
   switch(level)
   {
   case Logger::Level::Debug:
      return "DEBUG";
   case Logger::Level::Info:
      return "INFO ";
   case Logger::Level::Warn:
      return "WARN ";
   case Logger::Level::Error:
      return "ERROR";
   }
   return "?????";
}

// Wall-clock timestamp with microsecond resolution, e.g.
// [2026-07-20 14:03:22.123456]
void writeTimestamp(std::ostream& out)
{
   const auto now = std::chrono::system_clock::now();
   const auto time = std::chrono::system_clock::to_time_t(now);
   const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
   std::tm tmBuf{};
#if defined(_WIN32)
   localtime_s(&tmBuf, &time);
#else
   localtime_r(&time, &tmBuf);
#endif
   char buf[32];
   std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
   out << '[' << buf << '.' << std::setfill('0') << std::setw(6) << us.count() << "] ";
}

} // namespace

void StderrLogger::log(Level level, std::string_view message)
{
   std::lock_guard<detail::Mutex> lock(_mutex);
   writeTimestamp(std::cerr);
   std::cerr << '[' << toString(level) << "] " << message << '\n';
}
#else  // freestanding: no iostream/wall-clock. StderrLogger is a no-op sink.
void StderrLogger::log(Level, std::string_view) {}
#endif // GRIPPERS_BUILD_DEFAULT_SERIAL

void NullLogger::log(Level, std::string_view) {}

std::shared_ptr<Logger> makeDefaultLogger()
{
#if GRIPPERS_BUILD_DEFAULT_SERIAL
   // One shared instance: fallback users share a single mutex, so their
   // stderr lines stay serialized against each other.
   static const auto instance = std::make_shared<StderrLogger>();
#else
   // No console on a freestanding target; default to discarding. Pass an
   // application Logger (e.g. a UART sink) to get real logs.
   static const std::shared_ptr<Logger> instance = std::make_shared<NullLogger>();
#endif
   return instance;
}
} // namespace Robotiq
