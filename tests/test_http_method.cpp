//----------------------------------------
// File:      test_http_method.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "http_method.hpp"

#include <catch2/catch_test_macros.hpp>

using requestman::HttpMethod;
using requestman::to_string;
using requestman::try_parse_http_method;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("http method round-trip", "[domain][method]") {
  HttpMethod method{};
  REQUIRE(try_parse_http_method("GET", method));
  REQUIRE(method == HttpMethod::Get);
  REQUIRE(to_string(method) == "GET");

  REQUIRE(try_parse_http_method("patch", method));
  REQUIRE(method == HttpMethod::Patch);
  REQUIRE(to_string(HttpMethod::Options) == "OPTIONS");
  REQUIRE_FALSE(try_parse_http_method("TRACE", method));
}
