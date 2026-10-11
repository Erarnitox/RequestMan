//----------------------------------------
// File:      curl_identity.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "browser_presets.hpp"
#include "error.hpp"
#include "http_header.hpp"

#include <curl/curl.h>
#include <expected>
#include <string>
#include <vector>

namespace requestman {

struct IdentityApplication {
  std::string user_agent; // set CURLOPT_USERAGENT when non-empty
  std::vector<HttpHeader> extra_headers;
};

[[nodiscard]] bool curl_impersonate_available() noexcept;

[[nodiscard]] std::expected<IdentityApplication, Error>
prepare_client_identity(const ClientIdentity& identity,
                        const std::vector<HttpHeader>& existing_headers);

// Applies curl_easy_impersonate when configured. Returns an error if a browser
// preset was requested but impersonate support is unavailable/failed.
[[nodiscard]] std::expected<void, Error> apply_curl_impersonate(CURL* easy,
                                                                const ClientIdentity& identity);

} // namespace requestman
