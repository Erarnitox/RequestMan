//----------------------------------------
// File:      code_view.hpp
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
#include "raylib.h"

#include <string_view>

namespace requestman {

struct CodeViewState {
  Vector2 scroll{};
  int caret{0};       // byte offset
  int sel_anchor{-1}; // -1 = no selection; else range is [min,max) with caret
  bool dragging{false};
};

[[nodiscard]] inline bool code_view_has_selection(const CodeViewState& state) noexcept {
  return state.sel_anchor >= 0 && state.sel_anchor != state.caret;
}

[[nodiscard]] inline int code_view_sel_begin(const CodeViewState& state) noexcept {
  if(not code_view_has_selection(state)) {
    return state.caret;
  }
  return state.sel_anchor < state.caret ? state.sel_anchor : state.caret;
}

[[nodiscard]] inline int code_view_sel_end(const CodeViewState& state) noexcept {
  if(not code_view_has_selection(state)) {
    return state.caret;
  }
  return state.sel_anchor < state.caret ? state.caret : state.sel_anchor;
}

[[nodiscard]] std::string_view code_view_selected_text(const char* text,
                                                       const CodeViewState& state) noexcept;

// Editable code view with gutter + syntax highlighting. Returns true when focus should toggle
// (same convention as GuiTextBox).
[[nodiscard]] bool code_view(Rectangle bounds, char* text, int capacity, bool edit_mode,
                             CodeLanguage language, CodeViewState& state);

// Read-only code view (response panel): select + copy, no edit/paste.
void code_view(Rectangle bounds, const char* text, CodeLanguage language, CodeViewState& state);

} // namespace requestman
