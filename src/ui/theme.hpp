//----------------------------------------
// File:      theme.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_method.hpp"
#include "raylib.h"

#include <string_view>

namespace requestman::theme {

struct UiFont {
  Font font{};
  bool loaded{false};
};

struct CodeColors {
  Color gutter_bg{};
  Color gutter_text{};
  Color default_text{};
  Color string{};
  Color number{};
  Color keyword{};
  Color punctuation{};
  Color tag{};
  Color attribute{};
  Color comment{};
  Color invalid{};
  Color caret{};
  Color selection_bg{};
};

[[nodiscard]] Color page_background() noexcept;
[[nodiscard]] Color surface() noexcept;
[[nodiscard]] Color surface_raised() noexcept;
[[nodiscard]] Color border() noexcept;
[[nodiscard]] Color text() noexcept;
[[nodiscard]] Color text_muted() noexcept;
[[nodiscard]] Color primary() noexcept;
[[nodiscard]] Color primary_light() noexcept;
[[nodiscard]] Color success() noexcept;
[[nodiscard]] Color warning() noexcept;
[[nodiscard]] Color danger() noexcept;

[[nodiscard]] Color method_color(HttpMethod method) noexcept;
[[nodiscard]] Color status_color(int status_code) noexcept;

[[nodiscard]] CodeColors code_colors() noexcept;

[[nodiscard]] UiFont load_ui_font();
void unload_ui_font(UiFont& ui_font) noexcept;
void apply_theme(UiFont& ui_font);

// Rasterize any codepoints in `text` that are not already in the UI font atlas.
// Codepoints the font file does not contain are remembered so they are not retried.
void ensure_ui_glyphs(std::string_view text);

} // namespace requestman::theme
