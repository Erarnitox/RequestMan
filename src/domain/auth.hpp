//----------------------------------------
// File:      auth.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace requestman {

struct NoAuth {};

struct BearerAuth {
  std::string token;
};

struct BasicAuth {
  std::string username;
  std::string password;
};

struct ApiKeyAuth {
  enum class Location : unsigned { Header, Query };

  std::string key;
  std::string value;
  Location location{Location::Header};
  std::string header_name{"X-Api-Key"};
};

struct OAuth2ClientCredentialsAuth {
  std::string token_url;
  std::string client_id;
  std::string client_secret;
  std::string scope;          // optional
  std::string access_token;   // cached after fetch
  std::int64_t expires_at{0}; // unix seconds; 0 = unknown/none
};

using AuthConfig =
  std::variant<NoAuth, BearerAuth, BasicAuth, ApiKeyAuth, OAuth2ClientCredentialsAuth>;

} // namespace requestman
