//----------------------------------------
// File:      oauth2.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "auth.hpp"
#include "error.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace requestman {

inline constexpr std::int64_t kOAuth2ExpirySkewSeconds = 30;

[[nodiscard]] std::string form_url_encode(std::string_view value);

[[nodiscard]] std::string
build_client_credentials_form_body(const OAuth2ClientCredentialsAuth& auth,
                                   bool include_client_credentials_in_body);

struct OAuth2TokenResponse {
  std::string access_token;
  std::int64_t expires_in{0}; // 0 if absent
};

[[nodiscard]] std::expected<OAuth2TokenResponse, Error>
parse_oauth2_token_response(std::string_view json);

[[nodiscard]] bool oauth2_token_needs_refresh(const OAuth2ClientCredentialsAuth& auth,
                                              std::int64_t now_unix_seconds);

[[nodiscard]] inline bool oauth2_token_needs_refresh(const OAuth2ClientCredentialsAuth& auth) {
  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  return oauth2_token_needs_refresh(auth, now);
}

void apply_oauth2_token_response(OAuth2ClientCredentialsAuth& auth,
                                 const OAuth2TokenResponse& token, std::int64_t now_unix_seconds);

} // namespace requestman
