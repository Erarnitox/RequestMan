//----------------------------------------
// File:      oauth2_token.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "oauth2_token.hpp"

#include "curl_identity.hpp"
#include "curl_support.hpp"
#include "oauth2.hpp"
#include "url_util.hpp"

#include <chrono>
#include <curl/curl.h>
#include <exception>
#include <string>
#include <utility>
#include <variant>

namespace requestman {
namespace {

struct TokenWriteState {
  std::string* body{};
  bool overflow{false};
};

//----------------------------------------
//
//----------------------------------------
std::size_t write_token_body(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
  auto* state = static_cast<TokenWriteState*>(userdata);
  std::size_t total = 0;
  if(not curl_transfer_size(size, nmemb, total)) {
    state->overflow = true;
    return 0;
  }
  if(state->body->size() > kOAuth2TokenResponseMaxBytes
     || total > kOAuth2TokenResponseMaxBytes - state->body->size()) {
    state->overflow = true;
    return 0;
  }
  state->body->append(ptr, total);
  return total;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Error map_curl_error(CURLcode code, const char* errbuf) {
  const char* message =
    (errbuf != nullptr && errbuf[0] != '\0') ? errbuf : curl_easy_strerror(code);
  switch(code) {
  case CURLE_COULDNT_RESOLVE_HOST:
  case CURLE_COULDNT_CONNECT:
  case CURLE_OPERATION_TIMEDOUT:
  case CURLE_RECV_ERROR:
  case CURLE_SEND_ERROR:
    return make_error(ErrorKind::Transport, "OAuth2 token request failed", message);
  case CURLE_SSL_CONNECT_ERROR:
  case CURLE_SSL_CERTPROBLEM:
  case CURLE_SSL_CIPHER:
  case CURLE_SSL_CACERT_BADFILE:
    return make_error(ErrorKind::Transport, "OAuth2 TLS failure", message);
  case CURLE_URL_MALFORMAT:
    return make_error(ErrorKind::Validation, "malformed OAuth2 token URL", message);
  default:
    return make_error(ErrorKind::Transport, "OAuth2 transport error", message);
  }
}

} // namespace

//----------------------------------------
//
//----------------------------------------
std::expected<OAuth2ClientCredentialsAuth, Error>
fetch_oauth2_client_credentials_token(const OAuth2ClientCredentialsAuth& auth,
                                      const ClientIdentity& identity, const ProxySettings& proxy,
                                      std::chrono::milliseconds timeout) {
  if(auth.token_url.empty()) {
    return std::unexpected(make_error(ErrorKind::Validation, "OAuth2 token URL is required"));
  }
  if(auth.client_id.empty()) {
    return std::unexpected(make_error(ErrorKind::Validation, "OAuth2 client ID is required"));
  }
  if(proxy.enabled && not proxy.url.empty() && not proxy_scheme_allowed(proxy.url)) {
    return std::unexpected(
      make_error(ErrorKind::Validation, "proxy URL scheme is not allowed", proxy.url));
  }

  ensure_curl_global();

  const bool use_basic = not auth.client_secret.empty();
  // Prefer HTTP Basic when a secret is present, and also send body credentials for
  // servers that only accept form fields.
  const std::string form = build_client_credentials_form_body(auth, true);

  try {
    CurlEasy easy;
    CurlSlist headers;
    char errbuf[CURL_ERROR_SIZE]{};
    errbuf[0] = '\0';
    std::string response_body;
    TokenWriteState write_state{.body = &response_body};

    if(auto added =
         append_curl_header(headers, "Content-Type", "application/x-www-form-urlencoded");
       not added) {
      return std::unexpected(added.error());
    }
    if(auto added = append_curl_header(headers, "Accept", "application/json"); not added) {
      return std::unexpected(added.error());
    }

    curl_easy_setopt(easy.get(), CURLOPT_URL, auth.token_url.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(easy.get(), CURLOPT_POST, 1L);
    set_curl_upload_body(easy.get(), form);
    set_curl_timeout(easy.get(), timeout);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, write_token_body);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &write_state);
    lock_curl_to_http(easy.get());

    if(auto impersonated = apply_curl_impersonate(easy.get(), identity); not impersonated) {
      return std::unexpected(impersonated.error());
    }
    auto prepared_identity = prepare_client_identity(identity, {});
    if(not prepared_identity) {
      return std::unexpected(prepared_identity.error());
    }
    if(contains_crlf(prepared_identity->user_agent)) {
      return std::unexpected(make_error(ErrorKind::Validation, "user agent contains a line break"));
    }
    if(not prepared_identity->user_agent.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_USERAGENT, prepared_identity->user_agent.c_str());
    }
    for(const auto& header : prepared_identity->extra_headers) {
      if(auto added = append_curl_header(headers, header.name, header.value); not added) {
        return std::unexpected(added.error());
      }
    }
    if(headers.get() != nullptr) {
      curl_easy_setopt(easy.get(), CURLOPT_HTTPHEADER, headers.get());
    }
    if(proxy.enabled && not proxy.url.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_PROXY, proxy.url.c_str());
      if(not proxy.username.empty()) {
        curl_easy_setopt(easy.get(), CURLOPT_PROXYUSERNAME, proxy.username.c_str());
        curl_easy_setopt(easy.get(), CURLOPT_PROXYPASSWORD, proxy.password.c_str());
      }
    }

    if(use_basic) {
      curl_easy_setopt(easy.get(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
      curl_easy_setopt(easy.get(), CURLOPT_USERNAME, auth.client_id.c_str());
      curl_easy_setopt(easy.get(), CURLOPT_PASSWORD, auth.client_secret.c_str());
    }

    const CURLcode code = curl_easy_perform(easy.get());
    if(write_state.overflow) {
      return std::unexpected(
        make_error(ErrorKind::Transport, "OAuth2 token response exceeded 1 MiB"));
    }
    if(code != CURLE_OK) {
      return std::unexpected(map_curl_error(code, errbuf));
    }

    long status = 0;
    curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &status);
    if(status < 200 || status >= 300) {
      return std::unexpected(make_error(ErrorKind::Http, "OAuth2 token endpoint rejected request",
                                        "HTTP " + std::to_string(status)));
    }

    auto parsed = parse_oauth2_token_response(response_body);
    if(not parsed) {
      return std::unexpected(parsed.error());
    }

    OAuth2ClientCredentialsAuth updated = auth;
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
    apply_oauth2_token_response(updated, *parsed, now);
    return updated;
  } catch(const std::exception& ex) {
    return std::unexpected(
      make_error(ErrorKind::Transport, "OAuth2 token request failed", ex.what()));
  }
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> ensure_oauth2_access_token(HttpRequest& request) {
  auto* oauth = std::get_if<OAuth2ClientCredentialsAuth>(&request.auth);
  if(oauth == nullptr) {
    return {};
  }
  if(not oauth2_token_needs_refresh(*oauth)) {
    return {};
  }
  auto fetched = fetch_oauth2_client_credentials_token(
    *oauth, request.settings.identity, request.settings.proxy, request.settings.timeout);
  if(not fetched) {
    return std::unexpected(fetched.error());
  }
  request.auth = std::move(*fetched);
  return {};
}

} // namespace requestman
