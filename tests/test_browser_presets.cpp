//----------------------------------------
// File:      test_browser_presets.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "browser_presets.hpp"
#include "http_request.hpp"
#include "request_json.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("browser presets resolve impersonate targets and user agents", "[domain][identity]") {
  const auto& presets = browser_presets();
  REQUIRE(presets.size() >= 6);

  SECTION("none preset") {
    ClientIdentity identity{};
    identity.preset_id = "none";
    identity = resolve_client_identity(identity);
    REQUIRE(identity.impersonate.empty());
    REQUIRE_FALSE(identity.user_agent.empty());
    REQUIRE(identity.user_agent.find("RequestMan") != std::string::npos);
  }

  SECTION("chrome windows 11") {
    ClientIdentity identity{};
    identity.preset_id = "chrome_win11";
    identity = resolve_client_identity(identity);
    REQUIRE(identity.impersonate == "chrome146");
    REQUIRE(identity.user_agent.find("Windows NT 10.0") != std::string::npos);
    REQUIRE(identity.user_agent.find("Chrome/146") != std::string::npos);
    const auto* preset = find_browser_preset("chrome_win11");
    REQUIRE(preset != nullptr);
    REQUIRE(preset->sec_ch_ua_platform == "\"Windows\"");
  }

  SECTION("custom user agent is preserved") {
    ClientIdentity identity{};
    identity.preset_id = "firefox_win11";
    identity.user_agent = "custom-ua";
    identity = resolve_client_identity(identity);
    REQUIRE(identity.impersonate == "firefox147");
    REQUIRE(identity.user_agent == "custom-ua");
  }

  SECTION("dropdown labels include chrome windows 11") {
    const auto labels = browser_preset_dropdown_labels();
    REQUIRE(labels.find("Google Chrome Windows 11") != std::string::npos);
  }
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("client identity settings json round trip", "[database][identity]") {
  RequestSettings settings;
  settings.identity.preset_id = "chrome_win11";
  settings.identity.user_agent = "custom-chrome-ua";
  settings.identity.apply_browser_headers = false;
  settings.identity = resolve_client_identity(settings.identity);

  const auto json = serialize_settings(settings);
  auto restored = deserialize_settings(json);
  REQUIRE(restored);
  REQUIRE(restored->identity.preset_id == "chrome_win11");
  REQUIRE(restored->identity.impersonate == "chrome146");
  REQUIRE(restored->identity.user_agent == "custom-chrome-ua");
  REQUIRE_FALSE(restored->identity.apply_browser_headers);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("proxy settings json round trip", "[database][proxy]") {
  RequestSettings settings;
  settings.proxy.enabled = true;
  settings.proxy.url = "socks5://127.0.0.1:1080";
  settings.proxy.username = "proxy-user";
  settings.proxy.password = "proxy-pass";

  const auto json = serialize_settings(settings);
  auto restored = deserialize_settings(json);
  REQUIRE(restored);
  REQUIRE(restored->proxy.enabled);
  REQUIRE(restored->proxy.url == "socks5://127.0.0.1:1080");
  REQUIRE(restored->proxy.username == "proxy-user");
  REQUIRE(restored->proxy.password == "proxy-pass");
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("request json escapes control characters", "[database][json]") {
  HttpRequest request;
  request.url = "https://example.com";
  const std::string value = std::string{"a"} + '\x01' + "b";
  request.headers.push_back({.name = "X-Ctl", .value = value, .enabled = true});

  const auto json = serialize_request(request);
  REQUIRE(json.find('\x01') == std::string::npos);
  REQUIRE(json.find(R"(\u0001)") != std::string::npos);

  auto parsed = deserialize_request(json);
  REQUIRE(parsed);
  REQUIRE(parsed->headers.size() == 1);
  REQUIRE(parsed->headers[0].value == value);
}
