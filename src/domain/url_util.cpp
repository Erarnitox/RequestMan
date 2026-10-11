//----------------------------------------
// File:      url_util.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "url_util.hpp"

#include <cctype>
#include <sstream>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
std::string percent_encode(std::string_view value) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(value.size() * 3);
  for(char ch : value) {
    const auto c = static_cast<unsigned char>(ch);
    if(std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.' || c == '~') {
      out.push_back(ch);
    } else if(c == ' ') {
      out.push_back('+');
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0x0F]);
    }
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::string append_query_parameters(std::string_view base_url,
                                    const std::vector<HttpQueryParameter>& params) {
  const auto enabled = enabled_query_parameters(params);
  if(enabled.empty()) {
    return std::string{base_url};
  }

  std::string out{base_url};
  const bool has_query = out.find('?') != std::string::npos;
  bool first = not has_query;
  for(const auto& param : enabled) {
    out.push_back(first ? '?' : '&');
    first = false;
    out += percent_encode(param.name);
    out.push_back('=');
    out += percent_encode(param.value);
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
bool proxy_scheme_allowed(std::string_view url) noexcept {
  const auto separator = url.find("://");
  if(separator == std::string_view::npos) {
    return false;
  }
  const auto scheme = url.substr(0, separator);
  return equals_ci(scheme, "http") || equals_ci(scheme, "https") || equals_ci(scheme, "socks5")
         || equals_ci(scheme, "socks5h");
}

} // namespace requestman
