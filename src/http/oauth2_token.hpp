//----------------------------------------
// File:      oauth2_token.hpp
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
#include "browser_presets.hpp"
#include "error.hpp"
#include "http_request.hpp"

#include <chrono>
#include <expected>

namespace requestman {

inline constexpr std::size_t kOAuth2TokenResponseMaxBytes = 1024 * 1024;

[[nodiscard]] std::expected<OAuth2ClientCredentialsAuth, Error>
fetch_oauth2_client_credentials_token(const OAuth2ClientCredentialsAuth& auth,
                                      const ClientIdentity& identity = {},
                                      const ProxySettings& proxy = {},
                                      std::chrono::milliseconds timeout = std::chrono::seconds{30});

[[nodiscard]] std::expected<void, Error> ensure_oauth2_access_token(HttpRequest& request);

} // namespace requestman
