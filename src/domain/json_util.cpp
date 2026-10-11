//----------------------------------------
// File:      json_util.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "json_util.hpp"

#include <charconv>
#include <string>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool find_value_start(std::string_view json, std::string_view key,
                                    std::size_t& value_pos) {
  const std::string pattern = "\"" + std::string{key} + "\"";
  auto pos = json.find(pattern);
  if(pos == std::string_view::npos) {
    return false;
  }
  pos = json.find(':', pos + pattern.size());
  if(pos == std::string_view::npos) {
    return false;
  }
  ++pos;
  while(pos < json.size()
        && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) {
    ++pos;
  }
  if(pos >= json.size()) {
    return false;
  }
  value_pos = pos;
  return true;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int hex_digit(char ch) noexcept {
  if(ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if(ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  if(ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return -1;
}

//----------------------------------------
//
//----------------------------------------
void append_utf8(std::string& out, unsigned code) {
  if(code <= 0x7Fu) {
    out.push_back(static_cast<char>(code));
  } else if(code <= 0x7FFu) {
    out.push_back(static_cast<char>(0xC0u | (code >> 6)));
    out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
  } else {
    out.push_back(static_cast<char>(0xE0u | (code >> 12)));
    out.push_back(static_cast<char>(0x80u | ((code >> 6) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
  }
}

} // namespace

//----------------------------------------
//
//----------------------------------------
std::string json_escape(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for(const char ch : value) {
    const auto uc = static_cast<unsigned char>(ch);
    switch(ch) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if(uc < 0x20) {
        constexpr char kHex[] = "0123456789abcdef";
        out += "\\u00";
        out.push_back(kHex[uc >> 4]);
        out.push_back(kHex[uc & 0x0F]);
      } else {
        out.push_back(ch);
      }
      break;
    }
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
bool extract_json_string(std::string_view json, std::string_view key, std::string& out) {
  std::size_t pos = 0;
  if(not find_value_start(json, key, pos) || json[pos] != '"') {
    return false;
  }
  ++pos;
  std::string value;
  while(pos < json.size()) {
    const char ch = json[pos++];
    if(ch == '\\') {
      if(pos >= json.size()) {
        return false;
      }
      const char escaped = json[pos++];
      switch(escaped) {
      case '"':
      case '\\':
      case '/':
        value.push_back(escaped);
        break;
      case 'b':
        value.push_back('\b');
        break;
      case 'f':
        value.push_back('\f');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      case 'u': {
        if(pos + 4 > json.size()) {
          return false;
        }
        unsigned code = 0;
        for(int i = 0; i < 4; ++i) {
          const int digit = hex_digit(json[pos++]);
          if(digit < 0) {
            return false;
          }
          code = (code << 4) | static_cast<unsigned>(digit);
        }
        append_utf8(value, code);
        break;
      }
      default:
        value.push_back(escaped);
        break;
      }
    } else if(ch == '"') {
      out = std::move(value);
      return true;
    } else {
      value.push_back(ch);
    }
  }
  return false;
}

//----------------------------------------
//
//----------------------------------------
bool extract_json_int(std::string_view json, std::string_view key, long long& out) {
  std::size_t pos = 0;
  if(not find_value_start(json, key, pos)) {
    return false;
  }
  const auto* begin = json.data() + pos;
  const auto* end = json.data() + json.size();
  long long value = 0;
  auto [ptr, ec] = std::from_chars(begin, end, value);
  if(ec != std::errc{}) {
    return false;
  }
  out = value;
  return true;
}

//----------------------------------------
//
//----------------------------------------
bool extract_json_bool(std::string_view json, std::string_view key, bool& out) {
  std::size_t pos = 0;
  if(not find_value_start(json, key, pos)) {
    return false;
  }
  const auto rest = json.substr(pos);
  if(rest.starts_with("true")) {
    out = true;
    return true;
  }
  if(rest.starts_with("false")) {
    out = false;
    return true;
  }
  return false;
}

} // namespace requestman
