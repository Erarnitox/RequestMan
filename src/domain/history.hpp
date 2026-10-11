//----------------------------------------
// File:      history.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_method.hpp"
#include "http_request.hpp"
#include "ids.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace requestman {

struct HistoryEntry {
  HistoryEntryId id{};
  std::chrono::system_clock::time_point timestamp{};
  HttpMethod method{HttpMethod::Get};
  std::string url;
  HttpRequest request{};
  int status_code{0};
  std::chrono::milliseconds duration{0};
  std::size_t response_size{0};
  std::optional<std::string> error_message;
};

inline constexpr std::size_t kDefaultHistoryRetentionLimit = 500;

} // namespace requestman
