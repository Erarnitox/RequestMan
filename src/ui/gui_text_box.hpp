//----------------------------------------
// File:      gui_text_box.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

namespace requestman {

// Raygui keeps the text-box caret in the translation unit that implements it.
void set_gui_text_box_cursor(int index) noexcept;
[[nodiscard]] int gui_text_box_cursor() noexcept;

} // namespace requestman
