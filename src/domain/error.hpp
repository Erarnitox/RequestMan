//----------------------------------------
// File:      error.hpp
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
#include <utility>
#include <variant>

namespace requestman {

enum class ErrorKind : unsigned {
  Validation,
  Configuration,
  Storage,
  Transport,
  Http,
  Serialization,
  Cancellation,
  Unknown,
};

struct Error {
  ErrorKind kind{ErrorKind::Unknown};
  std::string message;
  std::string context;

  [[nodiscard]] std::string full_message() const {
    if(context.empty()) {
      return message;
    }
    return message + " (" + context + ")";
  }
};

[[nodiscard]] constexpr std::string_view to_string(ErrorKind kind) noexcept {
  switch(kind) {
  case ErrorKind::Validation:
    return "validation";
  case ErrorKind::Configuration:
    return "configuration";
  case ErrorKind::Storage:
    return "storage";
  case ErrorKind::Transport:
    return "transport";
  case ErrorKind::Http:
    return "http";
  case ErrorKind::Serialization:
    return "serialization";
  case ErrorKind::Cancellation:
    return "cancellation";
  case ErrorKind::Unknown:
    return "unknown";
  }
  return "unknown";
}

[[nodiscard]] inline Error make_error(ErrorKind kind, std::string message,
                                      std::string context = {}) {
  return Error{.kind = kind, .message = std::move(message), .context = std::move(context)};
}

} // namespace requestman
