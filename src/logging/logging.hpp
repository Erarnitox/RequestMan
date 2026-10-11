//----------------------------------------
// File:      logging.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <string>
#include <string_view>

namespace requestman {

enum class LogLevel : unsigned {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Critical,
};

void set_log_level(LogLevel level);
[[nodiscard]] LogLevel log_level() noexcept;

void log_message(LogLevel level, std::string_view message, std::string_view correlation_id = {});

inline void log_info(std::string_view message, std::string_view correlation_id = {}) {
  log_message(LogLevel::Info, message, correlation_id);
}
inline void log_warn(std::string_view message, std::string_view correlation_id = {}) {
  log_message(LogLevel::Warning, message, correlation_id);
}
inline void log_error(std::string_view message, std::string_view correlation_id = {}) {
  log_message(LogLevel::Error, message, correlation_id);
}
inline void log_debug(std::string_view message, std::string_view correlation_id = {}) {
  log_message(LogLevel::Debug, message, correlation_id);
}

// Redact common credential patterns before logging.
[[nodiscard]] std::string redact_for_log(std::string_view text);

} // namespace requestman
