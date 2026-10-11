//----------------------------------------
// File:      logging.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "logging.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <regex>

namespace requestman {
namespace {

std::atomic<LogLevel> g_level{LogLevel::Info};
std::mutex g_log_mutex;

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] const char* level_name(LogLevel level) noexcept {
  switch(level) {
  case LogLevel::Trace:
    return "TRACE";
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARN";
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::Critical:
    return "CRIT";
  }
  return "INFO";
}

} // namespace

//----------------------------------------
//
//----------------------------------------
void set_log_level(LogLevel level) {
  g_level.store(level, std::memory_order_relaxed);
}

//----------------------------------------
//
//----------------------------------------
LogLevel log_level() noexcept {
  return g_level.load(std::memory_order_relaxed);
}

//----------------------------------------
//
//----------------------------------------
std::string redact_for_log(std::string_view text) {
  std::string out{text};
  // ECMAScript std::regex does not support (?i); use std::regex::icase instead.
  struct Rule {
    std::regex pattern;
    const char* replacement;
  };
  static const Rule rules[] = {
    {std::regex{R"((://)([^/\s:@]+):([^/\s@]+)@)"}, "$1***:***@"},
    {std::regex{
       R"(([?&](?:access_token|refresh_token|id_token|client_secret|api[_-]?key|password|secret|token)=)([^&#\s]*))",
       std::regex::icase},
     "$1***"},
    {std::regex{R"((authorization\s*[:=]\s*)([^\s,;&#]+))", std::regex::icase}, "$1***"},
    {std::regex{R"((bearer\s+)([A-Za-z0-9\-._~+/]+=*))", std::regex::icase}, "$1***"},
    {std::regex{R"((api[_-]?key\s*[:=]\s*)([^\s,;&#]+))", std::regex::icase}, "$1***"},
    {std::regex{R"((password\s*[:=]\s*)([^\s,;&#]+))", std::regex::icase}, "$1***"},
    {std::regex{R"((token\s*[:=]\s*)([^\s,;&#]+))", std::regex::icase}, "$1***"},
  };
  for(const auto& rule : rules) {
    out = std::regex_replace(out, rule.pattern, rule.replacement);
  }
  return out;
}

namespace {

//----------------------------------------
//
//----------------------------------------
void append_log_escaped(std::string_view text) {
  constexpr char kHex[] = "0123456789abcdef";
  for(const char ch : text) {
    const auto c = static_cast<unsigned char>(ch);
    if(ch == '"' || ch == '\\') {
      std::cerr << '\\' << ch;
    } else if(c < 0x20 || c == 0x7F) {
      std::cerr << "\\u00" << kHex[c >> 4] << kHex[c & 0x0F];
    } else {
      std::cerr << ch;
    }
  }
}

} // namespace

//----------------------------------------
//
//----------------------------------------
void log_message(LogLevel level, std::string_view message, std::string_view correlation_id) {
  if(static_cast<unsigned>(level) < static_cast<unsigned>(log_level())) {
    return;
  }
  const auto safe = redact_for_log(message);
  const auto now = std::chrono::system_clock::now();
  const auto ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

  std::lock_guard lock{g_log_mutex};
  std::cerr << "{\"ts\":" << ms << ",\"level\":\"" << level_name(level) << "\"";
  if(not correlation_id.empty()) {
    std::cerr << ",\"cid\":\"";
    append_log_escaped(correlation_id);
    std::cerr << '"';
  }
  std::cerr << ",\"msg\":\"";
  append_log_escaped(safe);
  std::cerr << "\"}\n";
}

} // namespace requestman
