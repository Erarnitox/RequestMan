//----------------------------------------
// File:      test_auth_apply.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "auth.hpp"
#include "auth_apply.hpp"
#include "http_request.hpp"
#include "oauth2.hpp"
#include "request_json.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("authentication policies enrich requests", "[domain][auth]") {
  SECTION("bearer") {
    HttpRequest request;
    request.auth = BearerAuth{.token = "abc123"};
    auto enriched = apply_authentication(std::move(request));
    REQUIRE(enriched.headers.size() == 1);
    REQUIRE(enriched.headers.front().name == "Authorization");
    REQUIRE(enriched.headers.front().value == "Bearer abc123");
  }

  SECTION("basic") {
    HttpRequest request;
    request.auth = BasicAuth{.username = "user", .password = "pass"};
    auto enriched = apply_authentication(std::move(request));
    REQUIRE_FALSE(enriched.headers.empty());
    REQUIRE(enriched.headers.front().value.starts_with("Basic "));
  }

  SECTION("api key query") {
    HttpRequest request;
    request.auth =
      ApiKeyAuth{.key = "api_key", .value = "xyz", .location = ApiKeyAuth::Location::Query};
    auto enriched = apply_authentication(std::move(request));
    REQUIRE(enriched.query.size() == 1);
    REQUIRE(enriched.query.front().name == "api_key");
    REQUIRE(enriched.query.front().value == "xyz");
  }

  SECTION("oauth2 client credentials uses bearer access token") {
    HttpRequest request;
    request.auth = OAuth2ClientCredentialsAuth{.token_url = "https://auth.example/token",
                                               .client_id = "id",
                                               .client_secret = "secret",
                                               .scope = {},
                                               .access_token = "oauth-token",
                                               .expires_at = 0};
    auto enriched = apply_authentication(std::move(request));
    REQUIRE(enriched.headers.size() == 1);
    REQUIRE(enriched.headers.front().name == "Authorization");
    REQUIRE(enriched.headers.front().value == "Bearer oauth-token");
  }
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("oauth2 form body and token response parsing", "[domain][oauth2]") {
  SECTION("form url encode") {
    REQUIRE(form_url_encode("hello world") == "hello+world");
    REQUIRE(form_url_encode("a&b=c") == "a%26b%3Dc");
  }

  SECTION("client credentials body with credentials") {
    OAuth2ClientCredentialsAuth auth{.token_url = "https://example/token",
                                     .client_id = "cid",
                                     .client_secret = "csec",
                                     .scope = "read write",
                                     .access_token = {},
                                     .expires_at = 0};
    const auto body = build_client_credentials_form_body(auth, true);
    REQUIRE(body.find("grant_type=client_credentials") != std::string::npos);
    REQUIRE(body.find("client_id=cid") != std::string::npos);
    REQUIRE(body.find("client_secret=csec") != std::string::npos);
    REQUIRE(body.find("scope=read+write") != std::string::npos);
  }

  SECTION("client credentials body without embedding credentials") {
    OAuth2ClientCredentialsAuth auth{.token_url = {},
                                     .client_id = "cid",
                                     .client_secret = "csec",
                                     .scope = {},
                                     .access_token = {},
                                     .expires_at = 0};
    const auto body = build_client_credentials_form_body(auth, false);
    REQUIRE(body == "grant_type=client_credentials");
  }

  SECTION("parse token response") {
    const auto parsed = parse_oauth2_token_response(
      R"({"access_token":"tok-1","token_type":"Bearer","expires_in":3600})");
    REQUIRE(parsed);
    REQUIRE(parsed->access_token == "tok-1");
    REQUIRE(parsed->expires_in == 3600);
  }

  SECTION("parse token response requires access_token") {
    const auto parsed = parse_oauth2_token_response(R"({"expires_in":60})");
    REQUIRE_FALSE(parsed);
  }

  SECTION("needs refresh when missing or near expiry") {
    OAuth2ClientCredentialsAuth auth{.token_url = {},
                                     .client_id = {},
                                     .client_secret = {},
                                     .scope = {},
                                     .access_token = "tok",
                                     .expires_at = 1000};
    REQUIRE(oauth2_token_needs_refresh(auth, 1000 - kOAuth2ExpirySkewSeconds));
    REQUIRE_FALSE(oauth2_token_needs_refresh(auth, 1000 - kOAuth2ExpirySkewSeconds - 1));
    auth.access_token.clear();
    REQUIRE(oauth2_token_needs_refresh(auth, 0));
  }

  SECTION("apply token response sets expiry") {
    OAuth2ClientCredentialsAuth auth;
    apply_oauth2_token_response(auth, OAuth2TokenResponse{.access_token = "t", .expires_in = 120},
                                50);
    REQUIRE(auth.access_token == "t");
    REQUIRE(auth.expires_at == 170);
  }
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("oauth2 auth json round trip", "[database][auth]") {
  OAuth2ClientCredentialsAuth auth{.token_url = "https://auth.example/oauth/token",
                                   .client_id = "cid",
                                   .client_secret = "csec",
                                   .scope = "api",
                                   .access_token = "cached",
                                   .expires_at = 42};
  const auto json = serialize_auth(auth);
  auto restored = deserialize_auth(json);
  REQUIRE(restored);
  REQUIRE(std::holds_alternative<OAuth2ClientCredentialsAuth>(*restored));
  const auto& out = std::get<OAuth2ClientCredentialsAuth>(*restored);
  REQUIRE(out.token_url == auth.token_url);
  REQUIRE(out.client_id == auth.client_id);
  REQUIRE(out.client_secret == auth.client_secret);
  REQUIRE(out.scope == auth.scope);
  REQUIRE(out.access_token == auth.access_token);
  REQUIRE(out.expires_at == auth.expires_at);
}
