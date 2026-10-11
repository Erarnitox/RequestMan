//----------------------------------------
// File:      test_syntax_lexers.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "syntax_lexers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string_view>

using requestman::CodeLanguage;
using requestman::lex_json;
using requestman::lex_xml;
using requestman::sniff_code_language;
using requestman::TokenKind;

namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] TokenKind kind_at(const std::vector<requestman::SyntaxToken>& tokens,
                                std::size_t offset) {
  for(const auto& token : tokens) {
    if(offset >= token.offset && offset < token.offset + token.length) {
      return token.kind;
    }
  }
  return TokenKind::Default;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
TEST_CASE("lex_json covers strings numbers keywords and punctuation", "[syntax]") {
  constexpr std::string_view json = R"({"a":1,"b":true,"c":null})";
  const auto tokens = lex_json(json);
  REQUIRE_FALSE(tokens.empty());
  CHECK(kind_at(tokens, 0) == TokenKind::Punctuation); // {
  CHECK(kind_at(tokens, 1) == TokenKind::String);      // "a"
  CHECK(kind_at(tokens, 4) == TokenKind::Punctuation); // :
  CHECK(kind_at(tokens, 5) == TokenKind::Number);      // 1
  CHECK(kind_at(tokens, 11) == TokenKind::Keyword);    // true
  CHECK(kind_at(tokens, 20) == TokenKind::Keyword);    // null
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("lex_xml covers tags attributes comments and strings", "[syntax]") {
  constexpr std::string_view xml = R"(<!--c--><root id="1"/>text)";
  const auto tokens = lex_xml(xml);
  REQUIRE_FALSE(tokens.empty());
  CHECK(kind_at(tokens, 0) == TokenKind::Comment);
  CHECK(kind_at(tokens, 8) == TokenKind::Punctuation); // <
  CHECK(kind_at(tokens, 9) == TokenKind::Tag);         // root
  CHECK(kind_at(tokens, 14) == TokenKind::Attribute);  // id
  CHECK(kind_at(tokens, 17) == TokenKind::String);     // "1"
  CHECK(kind_at(tokens, 22) == TokenKind::Default);    // text
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("sniff_code_language prefers content-type then body", "[syntax]") {
  CHECK(sniff_code_language("application/json", "x") == CodeLanguage::Json);
  CHECK(sniff_code_language("text/html; charset=utf-8", "") == CodeLanguage::Xml);
  CHECK(sniff_code_language("", "  {\"a\":1}") == CodeLanguage::Json);
  CHECK(sniff_code_language("", "  <root/>") == CodeLanguage::Xml);
  CHECK(sniff_code_language("", "hello") == CodeLanguage::Plain);
}
