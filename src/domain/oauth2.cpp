//----------------------------------------
// File:      oauth2.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "oauth2.hpp"

#include "json_util.hpp"
#include "url_util.hpp"

#include <string>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
std::string form_url_encode(std::string_view value) {
  return percent_encode(value);
}

//----------------------------------------
//
//----------------------------------------
std::string build_client_credentials_form_body(const OAuth2ClientCredentialsAuth& auth,
                                               bool include_client_credentials_in_body) {
  std::string body = "grant_type=client_credentials";
  if(include_client_credentials_in_body) {
    body.append("&client_id=").append(form_url_encode(auth.client_id));
    if(not auth.client_secret.empty()) {
      body.append("&client_secret=").append(form_url_encode(auth.client_secret));
    }
  }
  if(not auth.scope.empty()) {
    body.append("&scope=").append(form_url_encode(auth.scope));
  }
  return body;
}

//----------------------------------------
//
//----------------------------------------
std::expected<OAuth2TokenResponse, Error> parse_oauth2_token_response(std::string_view json) {
  OAuth2TokenResponse token;
  if(not extract_json_string(json, "access_token", token.access_token)
     || token.access_token.empty()) {
    return std::unexpected(
      make_error(ErrorKind::Serialization, "OAuth2 token response missing access_token"));
  }
  long long expires_in = 0;
  if(extract_json_int(json, "expires_in", expires_in) && expires_in > 0) {
    token.expires_in = expires_in;
  }
  return token;
}

//----------------------------------------
//
//----------------------------------------
bool oauth2_token_needs_refresh(const OAuth2ClientCredentialsAuth& auth,
                                std::int64_t now_unix_seconds) {
  if(auth.access_token.empty()) {
    return true;
  }
  if(auth.expires_at <= 0) {
    return false;
  }
  return now_unix_seconds >= (auth.expires_at - kOAuth2ExpirySkewSeconds);
}

//----------------------------------------
//
//----------------------------------------
void apply_oauth2_token_response(OAuth2ClientCredentialsAuth& auth,
                                 const OAuth2TokenResponse& token, std::int64_t now_unix_seconds) {
  auth.access_token = token.access_token;
  if(token.expires_in > 0) {
    auth.expires_at = now_unix_seconds + token.expires_in;
  } else {
    auth.expires_at = 0;
  }
}

} // namespace requestman
