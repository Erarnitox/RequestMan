//----------------------------------------
// File:      code_language.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace requestman {

enum class CodeLanguage : unsigned {
  Plain = 0,
  Json,
  Xml,
};

enum class TokenKind : unsigned {
  Default = 0,
  String,
  Number,
  Keyword,
  Punctuation,
  Tag,
  Attribute,
  Comment,
  Invalid,
};

struct SyntaxToken {
  std::size_t offset{0};
  std::size_t length{0};
  TokenKind kind{TokenKind::Default};
};

inline constexpr std::size_t kMaxLexBytes = 256ULL * 1024ULL;

[[nodiscard]] CodeLanguage sniff_code_language(std::string_view content_type,
                                               std::string_view body) noexcept;

[[nodiscard]] CodeLanguage code_language_from_body_content_type(int body_content_type) noexcept;

} // namespace requestman
