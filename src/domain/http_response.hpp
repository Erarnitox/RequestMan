//----------------------------------------
// File:      http_response.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_header.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace requestman {

enum class BodyKind : unsigned {
  Empty,
  Text,
  Json,
  Binary,
};

struct HttpResponse {
  int status_code{0};
  std::string reason_phrase;
  std::vector<HttpHeader> headers;
  std::vector<std::byte> body;
  std::chrono::milliseconds elapsed{0};
  std::string final_url;
  BodyKind body_kind{BodyKind::Empty};
  bool truncated{false};
};

[[nodiscard]] inline std::size_t response_size_bytes(const HttpResponse& response) noexcept {
  return response.body.size();
}

} // namespace requestman
