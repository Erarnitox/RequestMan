//----------------------------------------
// File:      http_header.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace requestman {

struct HttpHeader {
  std::string name;
  std::string value;
  bool enabled{true};
};

struct HttpQueryParameter {
  std::string name;
  std::string value;
  bool enabled{true};
};

struct HttpCookie {
  std::string name;
  std::string value;
  bool enabled{true};
};

[[nodiscard]] inline std::vector<HttpHeader>
enabled_headers(const std::vector<HttpHeader>& headers) {
  std::vector<HttpHeader> out;
  out.reserve(headers.size());
  for(const auto& header : headers) {
    if(header.enabled && not header.name.empty()) {
      out.push_back(header);
    }
  }
  return out;
}

[[nodiscard]] inline bool equals_ci(std::string_view a, std::string_view b) noexcept {
  if(a.size() != b.size()) {
    return false;
  }
  for(std::size_t i = 0; i < a.size(); ++i) {
    const auto left = static_cast<unsigned char>(a[i]);
    const auto right = static_cast<unsigned char>(b[i]);
    if(std::tolower(left) != std::tolower(right)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool contains_ci(std::string_view hay, std::string_view needle) noexcept {
  if(needle.empty()) {
    return true;
  }
  if(needle.size() > hay.size()) {
    return false;
  }
  for(std::size_t i = 0; i + needle.size() <= hay.size(); ++i) {
    if(equals_ci(hay.substr(i, needle.size()), needle)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline bool contains_crlf(std::string_view text) noexcept {
  return text.find('\r') != std::string_view::npos || text.find('\n') != std::string_view::npos;
}

[[nodiscard]] inline bool header_present(const std::vector<HttpHeader>& headers,
                                         std::string_view name) noexcept {
  for(const auto& header : headers) {
    if(header.enabled && equals_ci(header.name, name)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] inline std::string_view header_value(const std::vector<HttpHeader>& headers,
                                                   std::string_view name) noexcept {
  for(const auto& header : headers) {
    if(equals_ci(header.name, name)) {
      return header.value;
    }
  }
  return {};
}

[[nodiscard]] inline std::vector<HttpQueryParameter>
enabled_query_parameters(const std::vector<HttpQueryParameter>& params) {
  std::vector<HttpQueryParameter> out;
  out.reserve(params.size());
  for(const auto& param : params) {
    if(param.enabled && not param.name.empty()) {
      out.push_back(param);
    }
  }
  return out;
}

} // namespace requestman
