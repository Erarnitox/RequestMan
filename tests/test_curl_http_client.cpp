//----------------------------------------
// File:      test_curl_http_client.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "http_client.hpp"
#include "http_request.hpp"
#include "url_util.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("curl client GET against httpbin", "[http][integration]") {
  auto client = make_curl_http_client();
  REQUIRE(client);

  HttpRequest request;
  request.method = HttpMethod::Get;
  request.url = "https://httpbin.org/get";
  request.query.push_back({.name = "q", .value = "requestman", .enabled = true});
  request.settings.timeout = std::chrono::seconds{20};

  auto response = client->send(request);
  REQUIRE(response);
  REQUIRE(response->status_code == 200);
  REQUIRE_FALSE(response->body.empty());
  REQUIRE(response->body_kind != BodyKind::Binary);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("curl client distinguishes transport failures", "[http][integration]") {
  auto client = make_curl_http_client();
  HttpRequest request;
  request.method = HttpMethod::Get;
  request.url = "https://this-host-should-not-resolve.invalid/";
  request.settings.timeout = std::chrono::seconds{5};
  auto response = client->send(request);
  REQUIRE_FALSE(response);
  REQUIRE(response.error().kind == ErrorKind::Transport);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("curl client rejects malformed URLs", "[http][integration]") {
  auto client = make_curl_http_client();
  HttpRequest request;
  request.url = "not a url";
  auto response = client->send(request);
  REQUIRE_FALSE(response);
  REQUIRE(response.error().kind == ErrorKind::Validation);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("curl client rejects header line breaks", "[http]") {
  auto client = make_curl_http_client();
  HttpRequest request;
  request.url = "https://example.com/";
  request.headers.push_back({.name = "X-Test", .value = "a\r\nX-Injected: yes", .enabled = true});

  auto response = client->send(request);
  REQUIRE_FALSE(response);
  REQUIRE(response.error().kind == ErrorKind::Validation);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("curl client rejects cookie line breaks", "[http]") {
  auto client = make_curl_http_client();
  HttpRequest request;
  request.url = "https://example.com/";
  request.cookies.push_back({.name = "session", .value = "a\nb", .enabled = true});

  auto response = client->send(request);
  REQUIRE_FALSE(response);
  REQUIRE(response.error().kind == ErrorKind::Validation);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("proxy schemes are limited to http, https, and socks", "[http]") {
  REQUIRE(proxy_scheme_allowed("http://127.0.0.1:8080"));
  REQUIRE(proxy_scheme_allowed("https://proxy.example"));
  REQUIRE(proxy_scheme_allowed("socks5://127.0.0.1:1080"));
  REQUIRE(proxy_scheme_allowed("socks5h://127.0.0.1:9050"));
  REQUIRE(proxy_scheme_allowed("SOCKS5H://127.0.0.1:9050"));
  REQUIRE_FALSE(proxy_scheme_allowed("file:///etc/passwd"));
  REQUIRE_FALSE(proxy_scheme_allowed("127.0.0.1:8080"));

  auto client = make_curl_http_client();
  HttpRequest request;
  request.url = "https://example.com/";
  request.settings.proxy.enabled = true;
  request.settings.proxy.url = "file:///etc/passwd";
  auto response = client->send(request);
  REQUIRE_FALSE(response);
  REQUIRE(response.error().kind == ErrorKind::Validation);
}
