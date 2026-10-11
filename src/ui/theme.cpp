//----------------------------------------
// File:      theme.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "theme.hpp"

#include "logging.hpp"
#include "raygui.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace requestman::theme {
namespace {

constexpr int kPage = static_cast<int>(0x1F2937FFu);
constexpr int kSurface = static_cast<int>(0x374151FFu);
constexpr int kSurfaceRaised = static_cast<int>(0x4B5563FFu);
constexpr int kBorder = static_cast<int>(0x4B5563FFu);
constexpr int kBorderFocus = static_cast<int>(0x2563EBFFu);
constexpr int kText = static_cast<int>(0xE5E7EBFFu);
constexpr int kTextMuted = static_cast<int>(0x9CA3AFFFu);
constexpr int kTextInverse = static_cast<int>(0x0F172AFFu);
constexpr int kAccent = static_cast<int>(0x2563EBFFu);
constexpr int kAccentPressed = static_cast<int>(0x1D4ED8FFu);
constexpr int kDisabledBorder = static_cast<int>(0x374151FFu);
constexpr int kDisabledBase = static_cast<int>(0x1F2937FFu);
constexpr int kDisabledText = static_cast<int>(0x6B7280FFu);

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string font_path() {
  return std::string{GetApplicationDirectory()}
         + "assets/fonts/JetBrainsMonoNerdFontMono-Regular.ttf";
}

} // namespace

//----------------------------------------
//
//----------------------------------------
Color page_background() noexcept {
  return Color{0x1F, 0x29, 0x37, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color surface() noexcept {
  return Color{0x37, 0x41, 0x51, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color surface_raised() noexcept {
  return Color{0x4B, 0x55, 0x63, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color border() noexcept {
  return Color{0x4B, 0x55, 0x63, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color text() noexcept {
  return Color{0xE5, 0xE7, 0xEB, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color text_muted() noexcept {
  return Color{0x9C, 0xA3, 0xAF, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color primary() noexcept {
  return Color{0x25, 0x63, 0xEB, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color primary_light() noexcept {
  return Color{0x60, 0xA5, 0xFA, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color success() noexcept {
  return Color{0x10, 0xB9, 0x81, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color warning() noexcept {
  return Color{0xF5, 0x9E, 0x0B, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color danger() noexcept {
  return Color{0xEF, 0x44, 0x44, 0xFF};
}

//----------------------------------------
//
//----------------------------------------
Color method_color(HttpMethod method) noexcept {
  switch(method) {
  case HttpMethod::Get:
    return success();
  case HttpMethod::Post:
    return warning();
  case HttpMethod::Put:
    return primary_light();
  case HttpMethod::Patch:
    return Color{0x8B, 0x5C, 0xF6, 0xFF};
  case HttpMethod::Delete:
    return danger();
  case HttpMethod::Head:
  case HttpMethod::Options:
  default:
    return text_muted();
  }
}

//----------------------------------------
//
//----------------------------------------
Color status_color(int status_code) noexcept {
  if(status_code >= 200 && status_code < 300)
    return success();
  if(status_code >= 300 && status_code < 400)
    return primary_light();
  if(status_code >= 400 && status_code < 500)
    return warning();
  if(status_code >= 500)
    return danger();
  return text_muted();
}

//----------------------------------------
//
//----------------------------------------
CodeColors code_colors() noexcept {
  return CodeColors{
    .gutter_bg = Color{0x11, 0x18, 0x27, 0xFF},
    .gutter_text = Color{0x6B, 0x72, 0x80, 0xFF},
    .default_text = Color{0xE5, 0xE7, 0xEB, 0xFF},
    .string = Color{0xF5, 0x9E, 0x0B, 0xFF},
    .number = Color{0x60, 0xA5, 0xFA, 0xFF},
    .keyword = Color{0x60, 0xA5, 0xFA, 0xFF},
    .punctuation = Color{0x9C, 0xA3, 0xAF, 0xFF},
    .tag = Color{0x60, 0xA5, 0xFA, 0xFF},
    .attribute = Color{0xF5, 0x9E, 0x0B, 0xFF},
    .comment = Color{0x6B, 0x72, 0x80, 0xFF},
    .invalid = Color{0xEF, 0x44, 0x44, 0xFF},
    .caret = Color{0xE5, 0xE7, 0xEB, 0xFF},
    .selection_bg = Color{0x37, 0x41, 0x51, 0xFF},
  };
}

namespace {

constexpr int kFontPx = 36;
constexpr int kFontPadding = 4;

struct Utf8Char {
  int codepoint{0};
  int bytes{0};
};

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Utf8Char next_utf8(const char* data, std::size_t avail) noexcept {
  if(avail == 0 || data == nullptr) {
    return {};
  }
  const auto lead = static_cast<unsigned char>(data[0]);
  if(lead < 0x80) {
    return {lead, 1};
  }

  int need = 0;
  int cp = 0;
  int min_cp = 0;
  if((lead & 0xE0) == 0xC0) {
    need = 2;
    cp = lead & 0x1F;
    min_cp = 0x80;
  } else if((lead & 0xF0) == 0xE0) {
    need = 3;
    cp = lead & 0x0F;
    min_cp = 0x800;
  } else if((lead & 0xF8) == 0xF0) {
    need = 4;
    cp = lead & 0x07;
    min_cp = 0x10000;
  } else {
    return {0xFFFD, 1};
  }

  if(static_cast<std::size_t>(need) > avail) {
    return {0xFFFD, 1};
  }
  for(int i = 1; i < need; ++i) {
    const auto cont = static_cast<unsigned char>(data[i]);
    if((cont & 0xC0) != 0x80) {
      return {0xFFFD, 1};
    }
    cp = (cp << 6) | (cont & 0x3F);
  }
  if(cp < min_cp || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
    return {0xFFFD, need};
  }
  return {cp, need};
}

//----------------------------------------
//
//----------------------------------------
void add_range(std::vector<int>& codepoints, int first, int last) {
  for(int cp = first; cp <= last; ++cp) {
    codepoints.push_back(cp);
  }
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::vector<int> base_codepoints() {
  std::vector<int> codepoints;
  codepoints.reserve(2200);
  add_range(codepoints, 0x20, 0x7E);
  add_range(codepoints, 0xA0, 0xFF);
  add_range(codepoints, 0x100, 0x24F);
  add_range(codepoints, 0x2000, 0x206F);
  add_range(codepoints, 0x20A0, 0x20CF);
  add_range(codepoints, 0x2190, 0x21FF);
  add_range(codepoints, 0x2200, 0x22FF);
  add_range(codepoints, 0x2300, 0x23FF);
  add_range(codepoints, 0x2500, 0x257F);
  add_range(codepoints, 0x2580, 0x259F);
  add_range(codepoints, 0x25A0, 0x25FF);
  add_range(codepoints, 0x2600, 0x26FF);
  add_range(codepoints, 0x2700, 0x27BF);
  return codepoints;
}

std::vector<unsigned char> g_font_file;
std::unordered_set<int> g_loaded;
std::unordered_set<int> g_missing;
UiFont* g_active = nullptr;

//----------------------------------------
//
//----------------------------------------
void reset_font_cache() noexcept {
  g_font_file.clear();
  g_loaded.clear();
  g_missing.clear();
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Font build_font(const std::vector<int>& requested) {
  Font font{};
  if(requested.empty() || g_font_file.empty() || g_font_file.size() > 0x7FFFFFFF) {
    return font;
  }

  std::vector<int> codepoints = requested;
  GlyphInfo* glyphs =
    LoadFontData(g_font_file.data(), static_cast<int>(g_font_file.size()), kFontPx,
                 codepoints.data(), static_cast<int>(codepoints.size()), FONT_DEFAULT);
  if(glyphs == nullptr) {
    return font;
  }

  std::vector<int> kept_cps;
  kept_cps.reserve(codepoints.size());
  std::size_t kept = 0;
  for(std::size_t i = 0; i < codepoints.size(); ++i) {
    if(glyphs[i].image.data == nullptr) {
      g_missing.insert(codepoints[i]);
      continue;
    }
    if(kept != i) {
      glyphs[kept] = glyphs[i];
    }
    kept_cps.push_back(codepoints[i]);
    ++kept;
  }

  if(kept == 0) {
    UnloadFontData(glyphs, 0);
    return font;
  }

  const int kept_count = static_cast<int>(kept);
  Rectangle* recs = nullptr;
  // Skyline packing uses each glyph's real height. Row packing (method 0) strides by
  // fontSize and overlaps box-drawing glyphs that extend past the em square.
  Image atlas = GenImageFontAtlas(glyphs, &recs, kept_count, kFontPx, kFontPadding, 1);
  font.texture = LoadTextureFromImage(atlas);
  if(font.texture.id == 0 || recs == nullptr) {
    UnloadFontData(glyphs, kept_count);
    MemFree(recs);
    UnloadImage(atlas);
    return font;
  }

  font.baseSize = kFontPx;
  font.glyphCount = kept_count;
  font.glyphPadding = kFontPadding;
  font.recs = recs;
  font.glyphs = glyphs;

  for(std::size_t i = 0; i < kept; ++i) {
    UnloadImage(glyphs[i].image);
    glyphs[i].image = ImageFromImage(atlas, recs[i]);
  }
  UnloadImage(atlas);
  SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

  g_loaded.clear();
  g_loaded.reserve(kept_cps.size());
  for(const int cp : kept_cps) {
    g_loaded.insert(cp);
  }
  return font;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
UiFont load_ui_font() {
  UiFont result{};
  reset_font_cache();

  const auto path = font_path();
  if(not FileExists(path.c_str())) {
    log_warn(std::string{"UI font not found at "} + path + "; using default font");
    return result;
  }

  int data_size = 0;
  unsigned char* file_data = LoadFileData(path.c_str(), &data_size);
  if(file_data == nullptr || data_size <= 0) {
    log_warn(std::string{"Failed to read UI font at "} + path + "; using default font");
    return result;
  }
  g_font_file.assign(file_data, file_data + data_size);
  UnloadFileData(file_data);

  result.font = build_font(base_codepoints());
  if(result.font.texture.id == 0) {
    log_warn("Failed to load UI font; using default font");
    reset_font_cache();
    return result;
  }

  result.loaded = true;
  log_info(std::string{"UI font loaded with "} + std::to_string(result.font.glyphCount)
           + " glyphs");
  return result;
}

//----------------------------------------
//
//----------------------------------------
void unload_ui_font(UiFont& ui_font) noexcept {
  if(g_active == &ui_font) {
    g_active = nullptr;
  }
  if(ui_font.loaded) {
    UnloadFont(ui_font.font);
    ui_font = {};
  }
  reset_font_cache();
}

//----------------------------------------
//
//----------------------------------------
void ensure_ui_glyphs(std::string_view text) {
  if(g_active == nullptr || not g_active->loaded || text.empty()) {
    return;
  }

  std::vector<int> extra;
  std::unordered_set<int> seen;
  const char* cursor = text.data();
  const char* end = cursor + text.size();
  while(cursor < end) {
    const Utf8Char decoded = next_utf8(cursor, static_cast<std::size_t>(end - cursor));
    if(decoded.bytes <= 0) {
      break;
    }
    cursor += decoded.bytes;
    const int cp = decoded.codepoint;
    if(cp < 32 || cp == 0x7F) {
      continue;
    }
    if(g_loaded.contains(cp) || g_missing.contains(cp) || not seen.insert(cp).second) {
      continue;
    }
    extra.push_back(cp);
  }
  if(extra.empty()) {
    return;
  }

  std::vector<int> requested;
  requested.reserve(g_loaded.size() + extra.size());
  for(const int cp : g_loaded) {
    requested.push_back(cp);
  }
  requested.insert(requested.end(), extra.begin(), extra.end());

  Font rebuilt = build_font(requested);
  if(rebuilt.texture.id == 0) {
    for(const int cp : extra) {
      g_missing.insert(cp);
    }
    log_warn("Failed to rebuild UI font atlas");
    return;
  }

  UnloadFont(g_active->font);
  g_active->font = rebuilt;
  GuiSetFont(rebuilt);
}

//----------------------------------------
//
//----------------------------------------
void apply_theme(UiFont& ui_font) {
  g_active = ui_font.loaded ? &ui_font : nullptr;
  GuiLoadStyleDefault();

  GuiSetStyle(DEFAULT, BACKGROUND_COLOR, kPage);
  GuiSetStyle(DEFAULT, LINE_COLOR, kBorder);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
  GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
  GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, 22);
  GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
  GuiSetStyle(DEFAULT, TEXT_PADDING, 8);

  GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, kSurface);
  GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, kText);

  GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, kBorderFocus);
  GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, kSurfaceRaised);
  GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, kText);

  GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, kAccentPressed);
  GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, kAccent);
  GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, kTextInverse);

  GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, kDisabledBorder);
  GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, kDisabledBase);
  GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, kDisabledText);

  GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, kTextMuted);
  GuiSetStyle(LABEL, TEXT_COLOR_FOCUSED, kText);
  GuiSetStyle(LABEL, TEXT_COLOR_PRESSED, kText);

  GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
  GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, kSurfaceRaised);
  GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, kText);
  GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, kAccent);
  GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, kAccent);
  GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, kTextInverse);
  GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED, kAccentPressed);
  GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, kAccentPressed);
  GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED, kTextInverse);

  GuiSetStyle(TEXTBOX, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(TEXTBOX, BASE_COLOR_NORMAL, kPage);
  GuiSetStyle(TEXTBOX, TEXT_COLOR_NORMAL, kText);
  GuiSetStyle(TEXTBOX, BORDER_COLOR_FOCUSED, kAccent);
  GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED, kSurface);
  GuiSetStyle(TEXTBOX, TEXT_COLOR_FOCUSED, kText);

  GuiSetStyle(COMBOBOX, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(COMBOBOX, BASE_COLOR_NORMAL, kSurfaceRaised);
  GuiSetStyle(COMBOBOX, TEXT_COLOR_NORMAL, kText);
  GuiSetStyle(COMBOBOX, BORDER_COLOR_FOCUSED, kAccent);
  GuiSetStyle(COMBOBOX, BASE_COLOR_FOCUSED, kSurfaceRaised);
  GuiSetStyle(COMBOBOX, TEXT_COLOR_FOCUSED, kText);

  GuiSetStyle(TOGGLE, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL, kSurface);
  GuiSetStyle(TOGGLE, TEXT_COLOR_NORMAL, kText);
  GuiSetStyle(TOGGLE, BORDER_COLOR_PRESSED, kAccent);
  GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED, kAccent);
  GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED, kTextInverse);
  GuiSetStyle(TOGGLE, GROUP_PADDING, 4);

  GuiSetStyle(DROPDOWNBOX, BORDER_COLOR_NORMAL, kBorder);
  GuiSetStyle(DROPDOWNBOX, BASE_COLOR_NORMAL, kSurfaceRaised);
  GuiSetStyle(DROPDOWNBOX, TEXT_COLOR_NORMAL, kText);
  GuiSetStyle(DROPDOWNBOX, BORDER_COLOR_FOCUSED, kAccent);
  GuiSetStyle(DROPDOWNBOX, BASE_COLOR_FOCUSED, kSurfaceRaised);
  GuiSetStyle(DROPDOWNBOX, TEXT_COLOR_FOCUSED, kText);
  GuiSetStyle(DROPDOWNBOX, BORDER_COLOR_PRESSED, kAccentPressed);
  GuiSetStyle(DROPDOWNBOX, BASE_COLOR_PRESSED, kAccent);
  GuiSetStyle(DROPDOWNBOX, TEXT_COLOR_PRESSED, kTextInverse);
  GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

  if(ui_font.loaded) {
    GuiSetFont(ui_font.font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
  }
}

} // namespace requestman::theme
