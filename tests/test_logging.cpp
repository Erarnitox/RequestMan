//----------------------------------------
// File:      test_logging.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "logging.hpp"

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <string>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("redact_for_log hides userinfo and query secrets", "[logging]") {
  const auto text =
    redact_for_log("https://alice:s3cret@example.com/path?token=abc&client_secret=xyz&x=1");
  REQUIRE(text.find("alice") == std::string::npos);
  REQUIRE(text.find("s3cret") == std::string::npos);
  REQUIRE(text.find("abc") == std::string::npos);
  REQUIRE(text.find("xyz") == std::string::npos);
  REQUIRE(text.find("example.com") != std::string::npos);
  REQUIRE(text.find("x=1") != std::string::npos);
  REQUIRE(text.find("://***:***@") != std::string::npos);
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("log_message escapes controls in the message and correlation id", "[logging]") {
  set_log_level(LogLevel::Info);
  std::stringstream captured;
  auto* previous = std::cerr.rdbuf(captured.rdbuf());
  log_message(LogLevel::Error, "say \"hi\"\nnext", "id\r1");
  std::cerr.rdbuf(previous);

  const auto text = captured.str();
  REQUIRE(text.find('\n') == text.size() - 1);
  REQUIRE(text.find("say \\\"hi\\\"") != std::string::npos);
  REQUIRE(text.find("\\u000a") != std::string::npos);
  REQUIRE(text.find("\\u000d") != std::string::npos);
  REQUIRE(text.find("id\r") == std::string::npos);
}
