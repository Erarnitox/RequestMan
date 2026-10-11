//----------------------------------------
// File:      syntax_lexers.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "code_language.hpp"

#include <string_view>
#include <vector>

namespace requestman {

[[nodiscard]] std::vector<SyntaxToken> lex_json(std::string_view text);
[[nodiscard]] std::vector<SyntaxToken> lex_xml(std::string_view text);
[[nodiscard]] std::vector<SyntaxToken> lex_code(std::string_view text, CodeLanguage language);

} // namespace requestman
