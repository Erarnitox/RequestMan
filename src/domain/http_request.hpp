//----------------------------------------
// File:      http_request.hpp
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
#include "http_header.hpp"
#include "http_method.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace requestman {

struct TlsSettings {
  bool verify_peer{true};
  bool verify_host{true};
  std::string ca_bundle_path;
  std::string client_cert_path;
  std::string client_key_path;
};

struct ProxySettings {
  bool enabled{false};
  std::string url;
  std::string username; // optional
  std::string password; // optional
};

struct RequestSettings {
  std::chrono::milliseconds timeout{std::chrono::seconds{30}};
  bool follow_redirects{true};
  int max_redirects{10};
  TlsSettings tls{};
  ProxySettings proxy{};
  ClientIdentity identity{};
  std::size_t max_response_bytes{32ULL * 1024ULL * 1024ULL}; // 32 MiB UI-safe default
};

struct HttpRequest {
  HttpMethod method{HttpMethod::Get};
  std::string url;
  std::vector<HttpQueryParameter> query;
  std::vector<HttpHeader> headers;
  std::vector<HttpCookie> cookies;
  std::string body;
  AuthConfig auth{NoAuth{}};
  RequestSettings settings{};
};

} // namespace requestman
