//----------------------------------------
// File:      icons.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "icons.hpp"

#include "logging.hpp"

#include <string>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string icon_file(const char* name) {
  std::string dir{GetApplicationDirectory()};
  if(not dir.empty() && dir.back() != '/' && dir.back() != '\\') {
    dir.push_back('/');
  }
  return dir + "assets/icons/" + name;
}

//----------------------------------------
//
//----------------------------------------
void load_one(UiIcons& icons, UiIcon id, const char* filename) {
  const auto path = icon_file(filename);
  if(not FileExists(path.c_str())) {
    log_warn(std::string{"Missing icon: "} + path);
    return;
  }
  // Load via Image so we can force RGBA — required for correct blending/tinting.
  Image image = LoadImage(path.c_str());
  if(image.data == nullptr) {
    log_warn(std::string{"Failed to load icon image: "} + path);
    return;
  }
  ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  const Texture2D tex = LoadTextureFromImage(image);
  UnloadImage(image);
  if(tex.id == 0) {
    log_warn(std::string{"Failed to create icon texture: "} + path);
    return;
  }
  SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
  const unsigned i = static_cast<unsigned>(id);
  icons.textures[i] = tex;
  icons.loaded[i] = true;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
UiIcons load_ui_icons() {
  UiIcons icons{};
  load_one(icons, UiIcon::LogoMark, "logo_mark.png");
  load_one(icons, UiIcon::Send, "icon_send.png");
  load_one(icons, UiIcon::Save, "icon_save.png");
  load_one(icons, UiIcon::Search, "icon_search.png");
  load_one(icons, UiIcon::History, "icon_history.png");
  load_one(icons, UiIcon::Trash, "icon_trash.png");
  load_one(icons, UiIcon::Copy, "icon_copy.png");
  load_one(icons, UiIcon::Beautify, "icon_beautify.png");

  const auto app_path = icon_file("app_icon_512.png");
  if(FileExists(app_path.c_str())) {
    icons.window_icon = LoadImage(app_path.c_str());
    icons.window_icon_loaded = icons.window_icon.data != nullptr;
    if(icons.window_icon_loaded) {
      // Ensure GLFW/SetWindowIcon receives an RGBA image with alpha.
      ImageFormat(&icons.window_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }
  } else {
    log_warn(std::string{"Missing window icon: "} + app_path);
  }
  return icons;
}

//----------------------------------------
//
//----------------------------------------
void unload_ui_icons(UiIcons& icons) noexcept {
  for(unsigned i = 0; i < static_cast<unsigned>(UiIcon::Count); ++i) {
    if(icons.loaded[i]) {
      UnloadTexture(icons.textures[i]);
      icons.loaded[i] = false;
      icons.textures[i] = {};
    }
  }
  if(icons.window_icon_loaded) {
    UnloadImage(icons.window_icon);
    icons.window_icon = {};
    icons.window_icon_loaded = false;
  }
}

//----------------------------------------
//
//----------------------------------------
void apply_window_icon(const UiIcons& icons) noexcept {
  if(icons.window_icon_loaded) {
    SetWindowIcon(icons.window_icon);
  }
}

//----------------------------------------
//
//----------------------------------------
bool ui_icon_ready(const UiIcons& icons, UiIcon id) noexcept {
  return icons.loaded[static_cast<unsigned>(id)];
}

//----------------------------------------
//
//----------------------------------------
void draw_ui_icon(const UiIcons& icons, UiIcon id, Rectangle dest, Color tint) noexcept {
  if(not ui_icon_ready(icons, id))
    return;
  const Texture2D& tex = icons.textures[static_cast<unsigned>(id)];
  // Explicit alpha blending so transparent icon backgrounds composite correctly.
  BeginBlendMode(BLEND_ALPHA);
  DrawTexturePro(tex,
                 Rectangle{0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                 dest, Vector2{0, 0}, 0.0f, tint);
  EndBlendMode();
}

} // namespace requestman
