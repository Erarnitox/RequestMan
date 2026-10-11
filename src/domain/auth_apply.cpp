//----------------------------------------
// File:      auth_apply.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "auth_apply.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string base64_encode(std::string_view input) {
  static constexpr char kAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((input.size() + 2) / 3) * 4);

  std::uint32_t buffer = 0;
  int bits = 0;
  for(char ch : input) {
    const auto c = static_cast<unsigned char>(ch);
    buffer = (buffer << 8) | c;
    bits += 8;
    while(bits >= 6) {
      bits -= 6;
      out.push_back(kAlphabet[(buffer >> bits) & 0x3F]);
    }
  }
  if(bits > 0) {
    out.push_back(kAlphabet[(buffer << (6 - bits)) & 0x3F]);
  }
  while(out.size() % 4 != 0) {
    out.push_back('=');
  }
  return out;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
HttpRequest apply_authentication(HttpRequest request) {
  std::visit(
    [&](auto&& auth) {
      using T = std::decay_t<decltype(auth)>;
      if constexpr(std::is_same_v<T, NoAuth>) {
        // nothing
      } else if constexpr(std::is_same_v<T, BearerAuth>) {
        if(not auth.token.empty()) {
          request.headers.push_back(
            {.name = "Authorization", .value = "Bearer " + auth.token, .enabled = true});
        }
      } else if constexpr(std::is_same_v<T, BasicAuth>) {
        const std::string credentials = auth.username + ":" + auth.password;
        request.headers.push_back({.name = "Authorization",
                                   .value = "Basic " + base64_encode(credentials),
                                   .enabled = true});
      } else if constexpr(std::is_same_v<T, ApiKeyAuth>) {
        if(auth.location == ApiKeyAuth::Location::Header) {
          request.headers.push_back(
            {.name = auth.header_name.empty() ? "X-Api-Key" : auth.header_name,
             .value = auth.value,
             .enabled = true});
        } else {
          request.query.push_back({.name = auth.key, .value = auth.value, .enabled = true});
        }
      } else if constexpr(std::is_same_v<T, OAuth2ClientCredentialsAuth>) {
        if(not auth.access_token.empty()) {
          request.headers.push_back(
            {.name = "Authorization", .value = "Bearer " + auth.access_token, .enabled = true});
        }
      }
    },
    request.auth);
  return request;
}

} // namespace requestman
