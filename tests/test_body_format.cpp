//----------------------------------------
// File:      test_body_format.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "body_format.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>
#include <vector>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("body detection and formatting", "[domain][body]") {
  SECTION("json detection and pretty print") {
    const std::string json = R"({"a":1,"b":[true,false]})";
    std::vector<std::byte> bytes(json.size());
    for(std::size_t i = 0; i < json.size(); ++i) {
      bytes[i] = static_cast<std::byte>(json[i]);
    }
    REQUIRE(detect_body_kind(bytes, "application/json") == BodyKind::Json);
    const auto pretty = format_json_pretty(json);
    REQUIRE(pretty.find('\n') != std::string::npos);
    REQUIRE(pretty.find("  ") != std::string::npos);
    REQUIRE(pretty.find("\"a\": 1") != std::string::npos);
  }

  SECTION("xml pretty print") {
    const std::string xml = R"(<?xml version="1.0"?><!--c--><root><a>1</a><b/></root>)";
    const auto pretty = format_xml_pretty(xml);
    REQUIRE(pretty.find('\n') != std::string::npos);
    REQUIRE(pretty.find("<?xml version=\"1.0\"?>") != std::string::npos);
    REQUIRE(pretty.find("<!--c-->") != std::string::npos);
    REQUIRE((pretty.find("  <a>1</a>") != std::string::npos
             || pretty.find("\n  <a>") != std::string::npos));
    REQUIRE(pretty.find("<b/>") != std::string::npos);
  }

  SECTION("binary detection") {
    std::vector<std::byte> bytes{std::byte{0x00}, std::byte{0x01}, std::byte{0x02}};
    REQUIRE(detect_body_kind(bytes) == BodyKind::Binary);
  }

  SECTION("display truncation") {
    HttpResponse response;
    response.body_kind = BodyKind::Text;
    response.body.assign(1000, std::byte{'x'});
    const auto text = format_response_for_display(response, false, 100);
    REQUIRE(text.size() < 200);
  }

  SECTION("pretty view beautifies html and xml") {
    const std::string html = "<html><body><p>Hi</p></body></html>";
    HttpResponse response;
    response.body_kind = BodyKind::Text;
    response.headers.push_back({.name = "Content-Type", .value = "text/html; charset=utf-8"});
    response.body.resize(html.size());
    for(std::size_t i = 0; i < html.size(); ++i) {
      response.body[i] = static_cast<std::byte>(html[i]);
    }

    const auto raw = format_response_for_display(response, false, 4096);
    const auto pretty = format_response_for_display(response, true, 4096);
    REQUIRE(raw.find('\n') == std::string::npos);
    REQUIRE(pretty.find("\n  <body>") != std::string::npos);
    REQUIRE(pretty.find("Hi") != std::string::npos);

    response.headers[0].value = "application/xml";
    response.body.clear();
    const std::string xml = "<root><a>1</a></root>";
    response.body.resize(xml.size());
    for(std::size_t i = 0; i < xml.size(); ++i) {
      response.body[i] = static_cast<std::byte>(xml[i]);
    }
    const auto xml_raw = format_response_for_display(response, false, 4096);
    const auto xml_pretty = format_response_for_display(response, true, 4096);
    REQUIRE(xml_raw.find('\n') == std::string::npos);
    REQUIRE(xml_pretty.find("\n  <a>") != std::string::npos);
  }
}
