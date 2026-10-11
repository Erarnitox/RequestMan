//----------------------------------------
// File:      syntax_lexers.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#include "syntax_lexers.hpp"

#include <cctype>
#include <string>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool is_json_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] char lower(char c) noexcept {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

//----------------------------------------
//
//----------------------------------------
void push_token(std::vector<SyntaxToken>& out, std::size_t offset, std::size_t length,
                TokenKind kind) {
  if(length == 0) {
    return;
  }
  if(not out.empty()) {
    auto& last = out.back();
    if(last.kind == kind && last.offset + last.length == offset) {
      last.length += length;
      return;
    }
  }
  out.push_back(SyntaxToken{.offset = offset, .length = length, .kind = kind});
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string_view clipped(std::string_view text) noexcept {
  if(text.size() > kMaxLexBytes) {
    return text.substr(0, kMaxLexBytes);
  }
  return text;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
CodeLanguage sniff_code_language(std::string_view content_type, std::string_view body) noexcept {
  {
    std::string lower_ct;
    lower_ct.reserve(content_type.size());
    for(char c : content_type) {
      lower_ct.push_back(lower(c));
    }
    if(lower_ct.find("json") != std::string::npos) {
      return CodeLanguage::Json;
    }
    if(lower_ct.find("xml") != std::string::npos || lower_ct.find("html") != std::string::npos) {
      return CodeLanguage::Xml;
    }
  }

  std::size_t i = 0;
  while(i < body.size()
        && (body[i] == ' ' || body[i] == '\t' || body[i] == '\r' || body[i] == '\n')) {
    ++i;
  }
  if(i >= body.size()) {
    return CodeLanguage::Plain;
  }
  if(body[i] == '{' || body[i] == '[') {
    return CodeLanguage::Json;
  }
  if(body[i] == '<') {
    return CodeLanguage::Xml;
  }
  return CodeLanguage::Plain;
}

//----------------------------------------
//
//----------------------------------------
CodeLanguage code_language_from_body_content_type(int body_content_type) noexcept {
  switch(body_content_type) {
  case 1:
    return CodeLanguage::Json;
  case 2:
    return CodeLanguage::Xml;
  default:
    return CodeLanguage::Plain;
  }
}

//----------------------------------------
//
//----------------------------------------
std::vector<SyntaxToken> lex_json(std::string_view text) {
  text = clipped(text);
  std::vector<SyntaxToken> out;
  out.reserve(text.size() / 4 + 8);
  std::size_t i = 0;
  while(i < text.size()) {
    const char c = text[i];
    if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      const std::size_t start = i;
      while(i < text.size()
            && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n')) {
        ++i;
      }
      push_token(out, start, i - start, TokenKind::Default);
      continue;
    }
    if(c == '"') {
      const std::size_t start = i++;
      bool escaped = false;
      while(i < text.size()) {
        if(escaped) {
          escaped = false;
          ++i;
          continue;
        }
        if(text[i] == '\\') {
          escaped = true;
          ++i;
          continue;
        }
        if(text[i] == '"') {
          ++i;
          break;
        }
        ++i;
      }
      push_token(out, start, i - start, TokenKind::String);
      continue;
    }
    if(c == '-' || is_json_digit(c)) {
      const std::size_t start = i;
      if(text[i] == '-') {
        ++i;
      }
      while(i < text.size() && is_json_digit(text[i])) {
        ++i;
      }
      if(i < text.size() && text[i] == '.') {
        ++i;
        while(i < text.size() && is_json_digit(text[i])) {
          ++i;
        }
      }
      if(i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        if(i < text.size() && (text[i] == '+' || text[i] == '-')) {
          ++i;
        }
        while(i < text.size() && is_json_digit(text[i])) {
          ++i;
        }
      }
      push_token(out, start, i - start, TokenKind::Number);
      continue;
    }
    if(c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',') {
      push_token(out, i, 1, TokenKind::Punctuation);
      ++i;
      continue;
    }
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      const std::size_t start = i;
      while(i < text.size()
            && ((text[i] >= 'a' && text[i] <= 'z') || (text[i] >= 'A' && text[i] <= 'Z')
                || is_json_digit(text[i]) || text[i] == '_')) {
        ++i;
      }
      const auto word = text.substr(start, i - start);
      if(word == "true" || word == "false" || word == "null") {
        push_token(out, start, i - start, TokenKind::Keyword);
      } else {
        push_token(out, start, i - start, TokenKind::Invalid);
      }
      continue;
    }
    push_token(out, i, 1, TokenKind::Invalid);
    ++i;
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::vector<SyntaxToken> lex_xml(std::string_view text) {
  text = clipped(text);
  std::vector<SyntaxToken> out;
  out.reserve(text.size() / 4 + 8);
  std::size_t i = 0;
  while(i < text.size()) {
    if(text[i] == '<' && i + 3 < text.size() && text[i + 1] == '!' && text[i + 2] == '-'
       && text[i + 3] == '-') {
      const std::size_t start = i;
      i += 4;
      while(i + 2 < text.size()
            && not(text[i] == '-' && text[i + 1] == '-' && text[i + 2] == '>')) {
        ++i;
      }
      if(i + 2 < text.size()) {
        i += 3;
      } else {
        i = text.size();
      }
      push_token(out, start, i - start, TokenKind::Comment);
      continue;
    }
    if(text[i] == '<' && i + 1 < text.size() && text[i + 1] == '?') {
      const std::size_t start = i;
      i += 2;
      while(i + 1 < text.size() && not(text[i] == '?' && text[i + 1] == '>')) {
        ++i;
      }
      if(i + 1 < text.size()) {
        i += 2;
      } else {
        i = text.size();
      }
      push_token(out, start, i - start, TokenKind::Punctuation);
      continue;
    }
    if(text[i] == '<') {
      const std::size_t start = i++;
      push_token(out, start, 1, TokenKind::Punctuation);
      if(i < text.size() && text[i] == '/') {
        push_token(out, i, 1, TokenKind::Punctuation);
        ++i;
      }
      const std::size_t name_start = i;
      while(i < text.size() && text[i] != ' ' && text[i] != '\t' && text[i] != '\r'
            && text[i] != '\n' && text[i] != '>' && text[i] != '/') {
        ++i;
      }
      push_token(out, name_start, i - name_start, TokenKind::Tag);
      while(i < text.size() && text[i] != '>') {
        if(text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n') {
          const std::size_t ws = i;
          while(i < text.size()
                && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n')) {
            ++i;
          }
          push_token(out, ws, i - ws, TokenKind::Default);
          continue;
        }
        if(text[i] == '/') {
          push_token(out, i, 1, TokenKind::Punctuation);
          ++i;
          continue;
        }
        if(text[i] == '"' || text[i] == '\'') {
          const char quote = text[i];
          const std::size_t qs = i++;
          while(i < text.size() && text[i] != quote) {
            ++i;
          }
          if(i < text.size()) {
            ++i;
          }
          push_token(out, qs, i - qs, TokenKind::String);
          continue;
        }
        if(text[i] == '=') {
          push_token(out, i, 1, TokenKind::Punctuation);
          ++i;
          continue;
        }
        const std::size_t attr = i;
        while(i < text.size() && text[i] != '=' && text[i] != ' ' && text[i] != '\t'
              && text[i] != '\r' && text[i] != '\n' && text[i] != '>' && text[i] != '/') {
          ++i;
        }
        push_token(out, attr, i - attr, TokenKind::Attribute);
      }
      if(i < text.size() && text[i] == '>') {
        push_token(out, i, 1, TokenKind::Punctuation);
        ++i;
      }
      continue;
    }
    const std::size_t start = i;
    while(i < text.size() && text[i] != '<') {
      ++i;
    }
    push_token(out, start, i - start, TokenKind::Default);
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::vector<SyntaxToken> lex_code(std::string_view text, CodeLanguage language) {
  switch(language) {
  case CodeLanguage::Json:
    return lex_json(text);
  case CodeLanguage::Xml:
    return lex_xml(text);
  case CodeLanguage::Plain:
  default:
    if(text.empty()) {
      return {};
    }
    const auto view = clipped(text);
    return {SyntaxToken{.offset = 0, .length = view.size(), .kind = TokenKind::Default}};
  }
}

} // namespace requestman
