//----------------------------------------
// File:      body_format.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "body_format.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace requestman {
namespace {

constexpr std::size_t kTextSniffSampleBytes = 4096;
constexpr std::size_t kTextSniffWeirdRatio = 20;

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool looks_like_text(std::span<const std::byte> body) {
  if(body.empty()) {
    return true;
  }
  std::size_t checked = 0;
  std::size_t weird = 0;
  for(auto b : body) {
    const auto c = static_cast<unsigned char>(b);
    ++checked;
    if(c == 0) {
      return false;
    }
    if(c < 0x09 || (c > 0x0D && c < 0x20 && c != 0x1B)) {
      ++weird;
    }
    if(checked >= kTextSniffSampleBytes) {
      break;
    }
  }
  return weird * kTextSniffWeirdRatio < checked;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool content_type_is_json(std::string_view content_type) {
  return content_type.find("json") != std::string_view::npos;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string_view response_content_type(const HttpResponse& response) noexcept {
  return header_value(response.headers, "content-type");
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool should_pretty_markup(std::string_view content_type,
                                        std::string_view text) noexcept {
  if(contains_ci(content_type, "html") || contains_ci(content_type, "xml")) {
    return true;
  }
  std::size_t i = 0;
  while(i < text.size() && std::isspace(static_cast<unsigned char>(text[i])) != 0) {
    ++i;
  }
  return i < text.size() && text[i] == '<';
}

} // namespace

//----------------------------------------
//
//----------------------------------------
BodyKind detect_body_kind(std::span<const std::byte> body, std::string_view content_type) {
  if(body.empty()) {
    return BodyKind::Empty;
  }
  if(not content_type.empty()) {
    if(content_type_is_json(content_type)) {
      return BodyKind::Json;
    }
    if(content_type.starts_with("text/") || content_type.find("xml") != std::string_view::npos
       || content_type.find("javascript") != std::string_view::npos
       || content_type.find("urlencoded") != std::string_view::npos) {
      return BodyKind::Text;
    }
    if(content_type.starts_with("image/") || content_type.starts_with("audio/")
       || content_type.starts_with("video/")
       || content_type.find("octet-stream") != std::string_view::npos) {
      return BodyKind::Binary;
    }
  }

  if(not looks_like_text(body)) {
    return BodyKind::Binary;
  }

  const auto text = body_as_lossy_text(body);
  auto first = text.find_first_not_of(" \t\r\n");
  if(first != std::string::npos && (text[first] == '{' || text[first] == '[')) {
    return BodyKind::Json;
  }
  return BodyKind::Text;
}

//----------------------------------------
//
//----------------------------------------
std::string body_as_lossy_text(std::span<const std::byte> body) {
  std::string out;
  out.resize(body.size());
  for(std::size_t i = 0; i < body.size(); ++i) {
    out[i] = static_cast<char>(body[i]);
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::string format_json_pretty(std::string_view json_text) {
  std::string out;
  out.reserve(json_text.size() + json_text.size() / 4);
  int indent = 0;
  bool in_string = false;
  bool escape = false;

  auto write_indent = [&] {
    for(int i = 0; i < indent; ++i) {
      out.append("  ");
    }
  };

  for(char c : json_text) {
    if(in_string) {
      out.push_back(c);
      if(escape) {
        escape = false;
      } else if(c == '\\') {
        escape = true;
      } else if(c == '"') {
        in_string = false;
      }
      continue;
    }

    switch(c) {
    case '"':
      in_string = true;
      out.push_back(c);
      break;
    case '{':
    case '[':
      out.push_back(c);
      out.push_back('\n');
      ++indent;
      write_indent();
      break;
    case '}':
    case ']':
      out.push_back('\n');
      --indent;
      if(indent < 0) {
        indent = 0;
      }
      write_indent();
      out.push_back(c);
      break;
    case ',':
      out.push_back(c);
      out.push_back('\n');
      write_indent();
      break;
    case ':':
      out.append(": ");
      break;
    default:
      if(std::isspace(static_cast<unsigned char>(c)) == 0) {
        out.push_back(c);
      }
      break;
    }
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::string format_xml_pretty(std::string_view xml_text) {
  std::string out;
  out.reserve(xml_text.size() + xml_text.size() / 4);
  int indent = 0;
  std::size_t i = 0;

  auto write_indent = [&] {
    for(int n = 0; n < indent; ++n) {
      out.append("  ");
    }
  };

  auto skip_ws = [&] {
    while(i < xml_text.size() && std::isspace(static_cast<unsigned char>(xml_text[i])) != 0) {
      ++i;
    }
  };

  auto emit_newline_indent = [&] {
    if(not out.empty() && out.back() != '\n') {
      out.push_back('\n');
    }
    write_indent();
  };

  while(i < xml_text.size()) {
    skip_ws();
    if(i >= xml_text.size()) {
      break;
    }

    if(xml_text[i] != '<') {
      // Text node: copy through next '<', trim edges.
      const std::size_t start = i;
      while(i < xml_text.size() && xml_text[i] != '<') {
        ++i;
      }
      std::string_view text = xml_text.substr(start, i - start);
      while(not text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
      }
      while(not text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1);
      }
      if(not text.empty()) {
        emit_newline_indent();
        out.append(text);
      }
      continue;
    }

    // Tag / special block starting at '<'
    if(i + 3 < xml_text.size() && xml_text.compare(i, 4, "<!--") == 0) {
      const std::size_t start = i;
      i += 4;
      while(i + 2 < xml_text.size()
            && not(xml_text[i] == '-' && xml_text[i + 1] == '-' && xml_text[i + 2] == '>')) {
        ++i;
      }
      i = std::min(i + 3, xml_text.size());
      emit_newline_indent();
      out.append(xml_text.substr(start, i - start));
      continue;
    }

    if(i + 8 < xml_text.size() && xml_text.compare(i, 9, "<![CDATA[") == 0) {
      const std::size_t start = i;
      i += 9;
      while(i + 2 < xml_text.size()
            && not(xml_text[i] == ']' && xml_text[i + 1] == ']' && xml_text[i + 2] == '>')) {
        ++i;
      }
      i = std::min(i + 3, xml_text.size());
      emit_newline_indent();
      out.append(xml_text.substr(start, i - start));
      continue;
    }

    if(i + 1 < xml_text.size() && xml_text[i + 1] == '?') {
      const std::size_t start = i;
      i += 2;
      while(i + 1 < xml_text.size() && not(xml_text[i] == '?' && xml_text[i + 1] == '>')) {
        ++i;
      }
      i = std::min(i + 2, xml_text.size());
      emit_newline_indent();
      out.append(xml_text.substr(start, i - start));
      continue;
    }

    const bool closing = i + 1 < xml_text.size() && xml_text[i + 1] == '/';
    const std::size_t start = i;
    ++i; // '<'
    if(closing) {
      ++i;
    }
    while(i < xml_text.size() && xml_text[i] != '>') {
      // Skip quoted attribute values so '>' inside quotes does not end the tag.
      if(xml_text[i] == '"' || xml_text[i] == '\'') {
        const char quote = xml_text[i++];
        while(i < xml_text.size() && xml_text[i] != quote) {
          ++i;
        }
        if(i < xml_text.size()) {
          ++i;
        }
        continue;
      }
      ++i;
    }
    bool self_closing = false;
    if(i < xml_text.size()) {
      // Look back for '/' before '>'
      std::size_t j = i;
      while(j > start && std::isspace(static_cast<unsigned char>(xml_text[j - 1])) != 0) {
        --j;
      }
      if(j > start && xml_text[j - 1] == '/') {
        self_closing = true;
      }
      ++i; // '>'
    }
    const auto tag = xml_text.substr(start, i - start);

    if(closing) {
      --indent;
      if(indent < 0) {
        indent = 0;
      }
      emit_newline_indent();
      out.append(tag);
    } else if(self_closing) {
      emit_newline_indent();
      out.append(tag);
    } else {
      emit_newline_indent();
      out.append(tag);
      ++indent;
    }
  }

  if(not out.empty() && out.back() != '\n') {
    out.push_back('\n');
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::string format_response_for_display(const HttpResponse& response, bool pretty,
                                        std::size_t max_chars) {
  if(response.body_kind == BodyKind::Binary) {
    return "<binary " + std::to_string(response.body.size()) + " bytes>";
  }
  if(response.body.empty()) {
    return {};
  }

  auto text = body_as_lossy_text(response.body);
  if(pretty && response.body_kind == BodyKind::Json) {
    text = format_json_pretty(text);
  } else if(pretty && should_pretty_markup(response_content_type(response), text)) {
    text = format_xml_pretty(text);
  }
  if(text.size() > max_chars) {
    text.resize(max_chars);
    text.append("\n… truncated for display …");
  }
  if(response.truncated) {
    text.append("\n… response truncated by size limit …");
  }
  return text;
}

} // namespace requestman
