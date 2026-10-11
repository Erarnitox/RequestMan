//----------------------------------------
// File:      curl_http_client.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "curl_http_client.hpp"

#include "auth_apply.hpp"
#include "body_format.hpp"
#include "curl_identity.hpp"
#include "curl_support.hpp"
#include "logging.hpp"
#include "url_util.hpp"

#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <exception>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace requestman {
namespace {

struct WriteState {
  std::vector<std::byte>* body{};
  std::size_t max_bytes{0};
  bool truncated{false};
};

//----------------------------------------
//
//----------------------------------------
std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
  auto* state = static_cast<WriteState*>(userdata);
  std::size_t total = 0;
  if(not curl_transfer_size(size, nmemb, total)) {
    return 0;
  }

  const std::size_t remaining =
    state->max_bytes > state->body->size() ? state->max_bytes - state->body->size() : 0;
  const std::size_t to_copy = std::min(total, remaining);
  const auto* bytes = reinterpret_cast<const std::byte*>(ptr);
  state->body->insert(state->body->end(), bytes, bytes + to_copy);
  if(to_copy < total) {
    state->truncated = true;
  }
  return total;
}

//----------------------------------------
//
//----------------------------------------
std::size_t header_callback(char* buffer, std::size_t size, std::size_t nitems, void* userdata) {
  auto* headers = static_cast<std::vector<HttpHeader>*>(userdata);
  std::size_t total = 0;
  if(not curl_transfer_size(size, nitems, total)) {
    return 0;
  }
  std::string_view line{buffer, total};
  while(not line.empty() && (line.back() == '\n' || line.back() == '\r')) {
    line.remove_suffix(1);
  }
  if(line.empty() || line.starts_with("HTTP/")) {
    return total;
  }
  const auto colon = line.find(':');
  if(colon == std::string_view::npos) {
    return total;
  }
  std::string name{line.substr(0, colon)};
  std::string_view value = line.substr(colon + 1);
  while(not value.empty() && value.front() == ' ') {
    value.remove_prefix(1);
  }
  headers->push_back({.name = std::move(name), .value = std::string{value}, .enabled = true});
  return total;
}

//----------------------------------------
//
//----------------------------------------
int progress_callback(void* clientp, curl_off_t /*dltotal*/, curl_off_t /*dlnow*/,
                      curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
  auto* cancel = static_cast<std::atomic_bool*>(clientp);
  if(cancel != nullptr && cancel->load(std::memory_order_relaxed)) {
    return 1;
  }
  return 0;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Error map_curl_error(CURLcode code, const char* errbuf) {
  std::string message = curl_easy_strerror(code);
  if(errbuf != nullptr && errbuf[0] != '\0') {
    message.append(": ");
    message.append(errbuf);
  }

  switch(code) {
  case CURLE_OPERATION_TIMEDOUT:
    return make_error(ErrorKind::Transport, "request timed out", message);
  case CURLE_COULDNT_RESOLVE_HOST:
  case CURLE_COULDNT_RESOLVE_PROXY:
    return make_error(ErrorKind::Transport, "DNS resolution failed", message);
  case CURLE_COULDNT_CONNECT:
    return make_error(ErrorKind::Transport, "connection failed", message);
  case CURLE_SSL_CONNECT_ERROR:
  case CURLE_PEER_FAILED_VERIFICATION:
  case CURLE_SSL_CERTPROBLEM:
  case CURLE_SSL_CIPHER:
  case CURLE_SSL_CACERT_BADFILE:
    return make_error(ErrorKind::Transport, "TLS failure", message);
  case CURLE_URL_MALFORMAT:
    return make_error(ErrorKind::Validation, "malformed URL", message);
  case CURLE_ABORTED_BY_CALLBACK:
    return make_error(ErrorKind::Cancellation, "request cancelled", message);
  default:
    return make_error(ErrorKind::Transport, "transport error", message);
  }
}

} // namespace

//----------------------------------------
//
//----------------------------------------
std::expected<HttpResponse, Error> CurlHttpClient::send(const HttpRequest& request,
                                                        std::atomic_bool* cancel_flag) {
  ensure_curl_global();

  if(request.url.empty()) {
    return std::unexpected(make_error(ErrorKind::Validation, "URL is required"));
  }

  auto prepared = apply_authentication(request);
  const std::string url = append_query_parameters(prepared.url, prepared.query);

  log_debug(redact_for_log(std::string{to_string(prepared.method)} + " " + url));

  try {
    CurlEasy easy;
    CurlSlist header_list;
    char errbuf[CURL_ERROR_SIZE]{};
    errbuf[0] = '\0';

    HttpResponse response;
    WriteState write_state{.body = &response.body,
                           .max_bytes = prepared.settings.max_response_bytes};

    curl_easy_setopt(easy.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(easy.get(), CURLOPT_FOLLOWLOCATION,
                     prepared.settings.follow_redirects ? 1L : 0L);
    curl_easy_setopt(easy.get(), CURLOPT_MAXREDIRS,
                     static_cast<long>(prepared.settings.max_redirects));
    set_curl_timeout(easy.get(), prepared.settings.timeout);
    if(auto impersonated = apply_curl_impersonate(easy.get(), prepared.settings.identity);
       not impersonated) {
      return std::unexpected(impersonated.error());
    }
    auto identity = prepare_client_identity(prepared.settings.identity, prepared.headers);
    if(not identity) {
      return std::unexpected(identity.error());
    }

    curl_easy_setopt(easy.get(), CURLOPT_ACCEPT_ENCODING, "");
    if(contains_crlf(identity->user_agent)) {
      return std::unexpected(make_error(ErrorKind::Validation, "user agent contains a line break"));
    }
    if(not identity->user_agent.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_USERAGENT, identity->user_agent.c_str());
    }
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &write_state);
    curl_easy_setopt(easy.get(), CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(easy.get(), CURLOPT_HEADERDATA, &response.headers);
    curl_easy_setopt(easy.get(), CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(easy.get(), CURLOPT_XFERINFODATA, cancel_flag);
    curl_easy_setopt(easy.get(), CURLOPT_NOPROGRESS, 0L);
    lock_curl_to_http(easy.get());
    curl_easy_setopt(easy.get(), CURLOPT_SSL_VERIFYPEER,
                     prepared.settings.tls.verify_peer ? 1L : 0L);
    curl_easy_setopt(easy.get(), CURLOPT_SSL_VERIFYHOST,
                     prepared.settings.tls.verify_host ? 2L : 0L);

    if(not prepared.settings.tls.ca_bundle_path.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_CAINFO, prepared.settings.tls.ca_bundle_path.c_str());
    }
    if(not prepared.settings.tls.client_cert_path.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_SSLCERT, prepared.settings.tls.client_cert_path.c_str());
    }
    if(not prepared.settings.tls.client_key_path.empty()) {
      curl_easy_setopt(easy.get(), CURLOPT_SSLKEY, prepared.settings.tls.client_key_path.c_str());
    }
    if(prepared.settings.proxy.enabled && not prepared.settings.proxy.url.empty()) {
      if(not proxy_scheme_allowed(prepared.settings.proxy.url)) {
        return std::unexpected(make_error(ErrorKind::Validation, "proxy URL scheme is not allowed",
                                          prepared.settings.proxy.url));
      }
      curl_easy_setopt(easy.get(), CURLOPT_PROXY, prepared.settings.proxy.url.c_str());
      if(not prepared.settings.proxy.username.empty()) {
        curl_easy_setopt(easy.get(), CURLOPT_PROXYUSERNAME,
                         prepared.settings.proxy.username.c_str());
        curl_easy_setopt(easy.get(), CURLOPT_PROXYPASSWORD,
                         prepared.settings.proxy.password.c_str());
      }
    }

    if(not prepared.settings.tls.verify_peer || not prepared.settings.tls.verify_host) {
      log_warn("insecure TLS settings enabled for request");
    }

    switch(prepared.method) {
    case HttpMethod::Get:
      curl_easy_setopt(easy.get(), CURLOPT_HTTPGET, 1L);
      break;
    case HttpMethod::Post:
      curl_easy_setopt(easy.get(), CURLOPT_POST, 1L);
      set_curl_upload_body(easy.get(), prepared.body);
      break;
    case HttpMethod::Put:
      curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, "PUT");
      set_curl_upload_body(easy.get(), prepared.body);
      break;
    case HttpMethod::Patch:
      curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, "PATCH");
      set_curl_upload_body(easy.get(), prepared.body);
      break;
    case HttpMethod::Delete:
      curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
      if(not prepared.body.empty()) {
        set_curl_upload_body(easy.get(), prepared.body);
      }
      break;
    case HttpMethod::Head:
      curl_easy_setopt(easy.get(), CURLOPT_NOBODY, 1L);
      curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, "HEAD");
      break;
    case HttpMethod::Options:
      curl_easy_setopt(easy.get(), CURLOPT_CUSTOMREQUEST, "OPTIONS");
      break;
    }

    for(const auto& header : identity->extra_headers) {
      if(auto added = append_curl_header(header_list, header.name, header.value); not added) {
        return std::unexpected(added.error());
      }
    }
    for(const auto& header : enabled_headers(prepared.headers)) {
      if(auto added = append_curl_header(header_list, header.name, header.value); not added) {
        return std::unexpected(added.error());
      }
    }
    if(not prepared.cookies.empty()) {
      std::string cookie_header;
      for(const auto& cookie : prepared.cookies) {
        if(not cookie.enabled || cookie.name.empty()) {
          continue;
        }
        if(contains_crlf(cookie.name) || contains_crlf(cookie.value)) {
          return std::unexpected(
            make_error(ErrorKind::Validation, "cookie contains a line break", cookie.name));
        }
        if(not cookie_header.empty()) {
          cookie_header.append("; ");
        }
        cookie_header.append(cookie.name).append("=").append(cookie.value);
      }
      if(not cookie_header.empty()) {
        if(auto added = append_curl_header(header_list, "Cookie", cookie_header); not added) {
          return std::unexpected(added.error());
        }
      }
    }
    if(header_list.get() != nullptr) {
      curl_easy_setopt(easy.get(), CURLOPT_HTTPHEADER, header_list.get());
    }

    const auto started = std::chrono::steady_clock::now();
    const CURLcode code = curl_easy_perform(easy.get());
    const auto ended = std::chrono::steady_clock::now();
    response.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ended - started);
    response.truncated = write_state.truncated;

    if(code != CURLE_OK) {
      return std::unexpected(map_curl_error(code, errbuf));
    }

    long status = 0;
    curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &status);
    response.status_code = static_cast<int>(status);

    char* effective = nullptr;
    curl_easy_getinfo(easy.get(), CURLINFO_EFFECTIVE_URL, &effective);
    if(effective != nullptr) {
      response.final_url = effective;
    }

    response.body_kind =
      detect_body_kind(response.body, header_value(response.headers, "content-type"));
    return response;
  } catch(const std::exception& ex) {
    return std::unexpected(make_error(ErrorKind::Transport, "curl client failure", ex.what()));
  }
}

//----------------------------------------
//
//----------------------------------------
std::unique_ptr<IHttpClient> make_curl_http_client() {
  ensure_curl_global();
  return std::make_unique<CurlHttpClient>();
}

} // namespace requestman
