//----------------------------------------
// File:      http_method.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace requestman {

enum class HttpMethod : std::uint8_t {
  Get,
  Post,
  Put,
  Patch,
  Delete,
  Head,
  Options,
};

struct HttpMethodName {
  HttpMethod method;
  std::string_view name;
};

inline constexpr HttpMethodName kHttpMethods[] = {
  {HttpMethod::Get, "GET"},         {HttpMethod::Post, "POST"},     {HttpMethod::Put, "PUT"},
  {HttpMethod::Patch, "PATCH"},     {HttpMethod::Delete, "DELETE"}, {HttpMethod::Head, "HEAD"},
  {HttpMethod::Options, "OPTIONS"},
};

[[nodiscard]] constexpr char ascii_lower(char ch) noexcept {
  if(ch >= 'A' && ch <= 'Z') {
    return static_cast<char>(ch - 'A' + 'a');
  }
  return ch;
}

[[nodiscard]] constexpr std::string_view to_string(HttpMethod method) noexcept {
  for(const auto& entry : kHttpMethods) {
    if(entry.method == method) {
      return entry.name;
    }
  }
  return "GET";
}

[[nodiscard]] constexpr bool try_parse_http_method(std::string_view text,
                                                   HttpMethod& out) noexcept {
  for(const auto& entry : kHttpMethods) {
    if(text.size() != entry.name.size()) {
      continue;
    }
    bool match = true;
    for(std::size_t i = 0; i < text.size(); ++i) {
      if(ascii_lower(text[i]) != ascii_lower(entry.name[i])) {
        match = false;
        break;
      }
    }
    if(match) {
      out = entry.method;
      return true;
    }
  }
  return false;
}

} // namespace requestman
