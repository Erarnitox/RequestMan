//----------------------------------------
// File:      curl_identity.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "curl_identity.hpp"

namespace requestman {

//----------------------------------------
//
//----------------------------------------
bool curl_impersonate_available() noexcept {
#if REQUESTMAN_HAS_CURL_IMPERSONATE
  return true;
#else
  return false;
#endif
}

//----------------------------------------
//
//----------------------------------------
std::expected<IdentityApplication, Error>
prepare_client_identity(const ClientIdentity& identity,
                        const std::vector<HttpHeader>& existing_headers) {
  ClientIdentity resolved = resolve_client_identity(identity);
  IdentityApplication out;

  if(not resolved.impersonate.empty() && not curl_impersonate_available()) {
    return std::unexpected(make_error(ErrorKind::Configuration,
                                      "browser impersonation requires libcurl-impersonate",
                                      "rebuild with REQUESTMAN_WITH_CURL_IMPERSONATE=ON"));
  }

  // Always surface an explicit UA for the none preset; for browser presets,
  // use the resolved UA (catalog default or user override).
  if(not resolved.user_agent.empty()) {
    out.user_agent = resolved.user_agent;
  }

  if(resolved.apply_browser_headers) {
    const BrowserPreset* preset = find_browser_preset(resolved.preset_id);
    if(preset != nullptr && not preset->sec_ch_ua_platform.empty()
       && not header_present(existing_headers, "sec-ch-ua-platform")) {
      out.extra_headers.push_back(HttpHeader{.name = "sec-ch-ua-platform",
                                             .value = std::string{preset->sec_ch_ua_platform},
                                             .enabled = true});
    }
  }

  return out;
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> apply_curl_impersonate(CURL* easy, const ClientIdentity& identity) {
  ClientIdentity resolved = resolve_client_identity(identity);
  if(resolved.impersonate.empty()) {
    return {};
  }
  if(not curl_impersonate_available()) {
    return std::unexpected(make_error(ErrorKind::Configuration,
                                      "browser impersonation requires libcurl-impersonate",
                                      "rebuild with REQUESTMAN_WITH_CURL_IMPERSONATE=ON"));
  }

#if REQUESTMAN_HAS_CURL_IMPERSONATE
  const int default_headers = resolved.apply_browser_headers ? 1 : 0;
  const CURLcode code = curl_easy_impersonate(easy, resolved.impersonate.c_str(), default_headers);
  if(code != CURLE_OK) {
    return std::unexpected(
      make_error(ErrorKind::Transport, "curl_easy_impersonate failed", curl_easy_strerror(code)));
  }
  return {};
#else
  (void)easy;
  return std::unexpected(
    make_error(ErrorKind::Configuration, "browser impersonation is not available in this build"));
#endif
}

} // namespace requestman
