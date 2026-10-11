//----------------------------------------
// File:      icons.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "raylib.h"

namespace requestman {

enum class UiIcon : unsigned {
  LogoMark = 0,
  Send,
  Save,
  Search,
  History,
  Trash,
  Copy,
  Beautify,
  Count
};

struct UiIcons {
  Texture2D textures[static_cast<unsigned>(UiIcon::Count)]{};
  bool loaded[static_cast<unsigned>(UiIcon::Count)]{};
  Image window_icon{};
  bool window_icon_loaded{false};
};

[[nodiscard]] UiIcons load_ui_icons();
void unload_ui_icons(UiIcons& icons) noexcept;
void apply_window_icon(const UiIcons& icons) noexcept;
void draw_ui_icon(const UiIcons& icons, UiIcon id, Rectangle dest, Color tint) noexcept;
[[nodiscard]] bool ui_icon_ready(const UiIcons& icons, UiIcon id) noexcept;

} // namespace requestman
