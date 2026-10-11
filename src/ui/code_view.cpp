//----------------------------------------
// File:      code_view.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "code_view.hpp"

#include "raygui.h"
#include "syntax_lexers.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace requestman {
namespace {

constexpr float kPad = 8.0f;
constexpr float kGutterPad = 6.0f;
constexpr float kLineGap = 4.0f;

CodeViewState* g_active_view = nullptr;

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] float line_height() {
  return static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SIZE)) + kLineGap;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int text_len(const char* text) {
  return static_cast<int>(std::strlen(text));
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int utf8_sequence_length(unsigned char lead) noexcept {
  if(lead < 0x80) {
    return 1;
  }
  if((lead & 0xE0) == 0xC0) {
    return 2;
  }
  if((lead & 0xF0) == 0xE0) {
    return 3;
  }
  if((lead & 0xF8) == 0xF0) {
    return 4;
  }
  return 1;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int codepoint_bytes_at(const char* text, int offset, int len) {
  if(text == nullptr || offset < 0 || offset >= len) {
    return 1;
  }
  const int sequence = utf8_sequence_length(static_cast<unsigned char>(text[offset]));
  if(sequence == 1 || offset + sequence > len) {
    return 1;
  }
  for(int i = 1; i < sequence; ++i) {
    if((static_cast<unsigned char>(text[offset + i]) & 0xC0) != 0x80) {
      return 1;
    }
  }
  return sequence;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int previous_codepoint_start(const char* text, int offset) {
  if(offset <= 0) {
    return 0;
  }
  int index = offset - 1;
  int guard = 0;
  while(index > 0 && guard < 3 && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) {
    --index;
    ++guard;
  }
  return index;
}

// Largest prefix of `text` (length `n`) that does not end in the middle of a codepoint.

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int utf8_prefix_bytes(const char* text, int n) {
  if(text == nullptr || n <= 0) {
    return 0;
  }
  int index = 0;
  while(index < n) {
    const int sequence = utf8_sequence_length(static_cast<unsigned char>(text[index]));
    if(index + sequence > n) {
      break;
    }
    bool complete = true;
    for(int i = 1; i < sequence; ++i) {
      if((static_cast<unsigned char>(text[index + i]) & 0xC0) != 0x80) {
        complete = false;
        break;
      }
    }
    if(not complete) {
      ++index;
      continue;
    }
    index += sequence;
  }
  return index;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int clamp_caret(int caret, int len) {
  return std::max(0, std::min(caret, len));
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool ctrl_down() {
  return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool shift_down() {
  return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Color color_for_token(TokenKind kind, const theme::CodeColors& colors) {
  switch(kind) {
  case TokenKind::String:
    return colors.string;
  case TokenKind::Number:
    return colors.number;
  case TokenKind::Keyword:
    return colors.keyword;
  case TokenKind::Punctuation:
    return colors.punctuation;
  case TokenKind::Tag:
    return colors.tag;
  case TokenKind::Attribute:
    return colors.attribute;
  case TokenKind::Comment:
    return colors.comment;
  case TokenKind::Invalid:
    return colors.invalid;
  case TokenKind::Default:
  default:
    return colors.default_text;
  }
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int count_lines(const char* text) {
  int lines = 1;
  for(const char* p = text; *p != '\0'; ++p) {
    if(*p == '\n') {
      ++lines;
    }
  }
  return lines;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] float measure_content_height(const char* text) {
  return static_cast<float>(count_lines(text)) * line_height() + kPad * 2.0f;
}

struct LineInfo {
  int line{0};
  int column{0};
  int line_start{0};
  int line_end{0};
};

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] LineInfo line_info_at(const char* text, int caret) {
  const int len = text_len(text);
  caret = clamp_caret(caret, len);
  LineInfo info{};
  int i = 0;
  while(i < caret) {
    if(text[i] == '\n') {
      ++info.line;
      info.line_start = i + 1;
    }
    ++i;
  }
  info.column = caret - info.line_start;
  info.line_end = caret;
  while(info.line_end < len && text[info.line_end] != '\n') {
    ++info.line_end;
  }
  return info;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int offset_at_line_col(const char* text, int line, int column) {
  const int len = text_len(text);
  int current = 0;
  int i = 0;
  while(i < len && current < line) {
    if(text[i] == '\n') {
      ++current;
    }
    ++i;
  }
  int col = 0;
  while(i < len && text[i] != '\n' && col < column) {
    ++i;
    ++col;
  }
  return i;
}

//----------------------------------------
//
//----------------------------------------
void clear_selection(CodeViewState& state) {
  state.sel_anchor = -1;
}

//----------------------------------------
//
//----------------------------------------
void ensure_anchor_for_shift(CodeViewState& state) {
  if(state.sel_anchor < 0) {
    state.sel_anchor = state.caret;
  }
}

//----------------------------------------
//
//----------------------------------------
void insert_at(char* text, int capacity, int& caret, char ch) {
  const int len = text_len(text);
  caret = clamp_caret(caret, len);
  if(len + 1 >= capacity) {
    return;
  }
  std::memmove(text + caret + 1, text + caret, static_cast<std::size_t>(len - caret) + 1);
  text[caret] = ch;
  ++caret;
}

//----------------------------------------
//
//----------------------------------------
void delete_before(char* text, int& caret) {
  const int len = text_len(text);
  caret = clamp_caret(caret, len);
  if(caret <= 0) {
    return;
  }
  const int start = previous_codepoint_start(text, caret);
  std::memmove(text + start, text + caret, static_cast<std::size_t>(len - caret) + 1);
  caret = start;
}

//----------------------------------------
//
//----------------------------------------
void delete_after(char* text, int& caret) {
  const int len = text_len(text);
  caret = clamp_caret(caret, len);
  if(caret >= len) {
    return;
  }
  const int sequence = codepoint_bytes_at(text, caret, len);
  std::memmove(text + caret, text + caret + sequence,
               static_cast<std::size_t>(len - caret - sequence) + 1);
}

//----------------------------------------
//
//----------------------------------------
void delete_range(char* text, int begin, int end, int& caret) {
  const int len = text_len(text);
  begin = clamp_caret(begin, len);
  end = clamp_caret(end, len);
  if(end <= begin) {
    return;
  }
  std::memmove(text + begin, text + end, static_cast<std::size_t>(len - end) + 1);
  caret = begin;
}

//----------------------------------------
//
//----------------------------------------
void delete_selection(char* text, CodeViewState& state) {
  if(not code_view_has_selection(state)) {
    return;
  }
  delete_range(text, code_view_sel_begin(state), code_view_sel_end(state), state.caret);
  clear_selection(state);
}

//----------------------------------------
//
//----------------------------------------
void insert_bytes(char* text, int capacity, CodeViewState& state, const char* bytes, int n) {
  if(n <= 0 || bytes == nullptr) {
    return;
  }
  if(code_view_has_selection(state)) {
    delete_selection(text, state);
  }
  for(int i = 0; i < n; ++i) {
    const unsigned char ch = static_cast<unsigned char>(bytes[i]);
    if(ch == '\r') {
      continue;
    }
    if(ch == '\n' || ch == '\t' || ch >= 32) {
      insert_at(text, capacity, state.caret, static_cast<char>(ch));
    }
  }
  clear_selection(state);
}

//----------------------------------------
//
//----------------------------------------
void copy_selection_to_clipboard(const char* text, const CodeViewState& state) {
  if(not code_view_has_selection(state)) {
    return;
  }
  const int begin = code_view_sel_begin(state);
  const int end = code_view_sel_end(state);
  const std::string slice(text + begin, text + end);
  SetClipboardText(slice.c_str());
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] float gutter_width_for(int line_count, Font font, float size, float spacing) {
  int digits = 1;
  for(int n = line_count; n >= 10; n /= 10) {
    ++digits;
  }
  digits = std::max(digits, 2);
  char sample[16];
  for(int i = 0; i < digits; ++i) {
    sample[i] = '9';
  }
  sample[digits] = '\0';
  const Vector2 m = MeasureTextEx(font, sample, size, spacing);
  return m.x + kGutterPad * 2.0f + 4.0f;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int caret_from_click(const char* text, Font font, float size, float spacing, float lh,
                                   float text_origin_x, float text_origin_y, Vector2 mouse) {
  const int lines = count_lines(text);
  int line = static_cast<int>((mouse.y - text_origin_y) / lh);
  line = std::max(0, std::min(line, lines - 1));

  const int line_start = offset_at_line_col(text, line, 0);
  const LineInfo info = line_info_at(text, line_start);
  const int line_len = info.line_end - info.line_start;

  float x = text_origin_x;
  int best = 0;
  float best_dist = std::fabs(mouse.x - x);
  const int text_end = text_len(text);
  int byte_index = 0;
  while(byte_index < line_len) {
    const int sequence = codepoint_bytes_at(text, info.line_start + byte_index, text_end);
    char buf[8]{};
    std::memcpy(buf, text + info.line_start + byte_index, static_cast<std::size_t>(sequence));
    x += MeasureTextEx(font, buf, size, spacing).x;
    byte_index += sequence;
    const float dist = std::fabs(mouse.x - x);
    if(dist < best_dist) {
      best_dist = dist;
      best = byte_index;
    }
  }
  return info.line_start + best;
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] float measure_prefix_width(const char* text, int line_start, int offset, Font font,
                                         float size, float spacing) {
  if(offset <= line_start) {
    return 0.0f;
  }
  char prefix[2048];
  const char* src = text + line_start;
  const int requested = std::min(offset - line_start, static_cast<int>(sizeof(prefix) - 1));
  const int n = utf8_prefix_bytes(src, requested);
  if(n <= 0) {
    return 0.0f;
  }
  std::memcpy(prefix, src, static_cast<std::size_t>(n));
  prefix[n] = '\0';
  return MeasureTextEx(font, prefix, size, spacing).x;
}

//----------------------------------------
//
//----------------------------------------
void move_caret(CodeViewState& state, int new_caret, int len, bool extend) {
  new_caret = clamp_caret(new_caret, len);
  if(extend) {
    ensure_anchor_for_shift(state);
  } else {
    clear_selection(state);
  }
  state.caret = new_caret;
}

//----------------------------------------
//
//----------------------------------------
void draw_gutter_and_text(Rectangle view, float gutter_w, Vector2 scroll, const char* text,
                          const std::vector<SyntaxToken>& tokens, const CodeViewState& state,
                          bool show_caret) {
  const auto colors = theme::code_colors();
  const Font font = GuiGetFont();
  const float size = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SIZE));
  const float spacing = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SPACING));
  const float lh = line_height();
  const int lines = count_lines(text);
  const float text_x0 = view.x + scroll.x + gutter_w + kPad;
  const float text_y0 = view.y + scroll.y + kPad;
  const bool has_sel = code_view_has_selection(state);
  const int sel_begin = code_view_sel_begin(state);
  const int sel_end = code_view_sel_end(state);

  DrawRectangle(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(gutter_w),
                static_cast<int>(view.height), colors.gutter_bg);

  const int first_line = std::max(0, static_cast<int>(std::floor((-scroll.y - kPad) / lh)));
  const int last_line =
    std::min(lines - 1, static_cast<int>(std::ceil((-scroll.y + view.height) / lh)) + 1);

  int offset = 0;
  int line = 0;
  while(line < first_line && text[offset] != '\0') {
    if(text[offset] == '\n') {
      ++line;
    }
    ++offset;
  }

  std::size_t token_i = 0;
  while(token_i < tokens.size()
        && tokens[token_i].offset + tokens[token_i].length <= static_cast<std::size_t>(offset)) {
    ++token_i;
  }

  for(int draw_line = first_line; draw_line <= last_line; ++draw_line) {
    const float y = text_y0 + static_cast<float>(draw_line) * lh;
    char num[16];
    std::snprintf(num, sizeof(num), "%d", draw_line + 1);
    const Vector2 nm = MeasureTextEx(font, num, size, spacing);
    DrawTextEx(font, num, Vector2{view.x + gutter_w - kGutterPad - nm.x, y}, size, spacing,
               colors.gutter_text);

    const int line_start = offset;
    int line_end = offset;
    while(text[line_end] != '\0' && text[line_end] != '\n') {
      ++line_end;
    }

    if(has_sel && sel_begin <= line_end && sel_end >= line_start) {
      const int seg_a = std::max(sel_begin, line_start);
      const int seg_b = std::min(sel_end, line_end);
      float x0 = text_x0 + measure_prefix_width(text, line_start, seg_a, font, size, spacing);
      float x1 = text_x0 + measure_prefix_width(text, line_start, seg_b, font, size, spacing);
      if(sel_end > line_end && text[line_end] == '\n') {
        x1 = std::max(x1, x0 + 6.0f);
      }
      if(seg_a == seg_b && sel_end > line_end) {
        x1 = std::max(x1, x0 + 6.0f);
      }
      if(x1 > x0) {
        DrawRectangle(static_cast<int>(x0), static_cast<int>(y), static_cast<int>(x1 - x0),
                      static_cast<int>(lh), colors.selection_bg);
      }
    }

    float x = text_x0;
    int pos = line_start;
    while(pos < line_end) {
      while(token_i < tokens.size()
            && tokens[token_i].offset + tokens[token_i].length <= static_cast<std::size_t>(pos)) {
        ++token_i;
      }
      TokenKind kind = TokenKind::Default;
      std::size_t run_end = static_cast<std::size_t>(line_end);
      if(token_i < tokens.size()) {
        const auto& tok = tokens[token_i];
        const auto tok_end = tok.offset + tok.length;
        if(static_cast<std::size_t>(pos) >= tok.offset && static_cast<std::size_t>(pos) < tok_end) {
          kind = tok.kind;
          run_end = std::min(run_end, tok_end);
        } else if(tok.offset > static_cast<std::size_t>(pos)) {
          run_end = std::min(run_end, tok.offset);
        }
      }
      const int n = static_cast<int>(run_end) - pos;
      if(n <= 0) {
        break;
      }
      char tmp[2048];
      const int copy =
        utf8_prefix_bytes(text + pos, std::min(n, static_cast<int>(sizeof(tmp) - 1)));
      if(copy <= 0) {
        break;
      }
      std::memcpy(tmp, text + pos, static_cast<std::size_t>(copy));
      tmp[copy] = '\0';
      DrawTextEx(font, tmp, Vector2{x, y}, size, spacing, color_for_token(kind, colors));
      x += MeasureTextEx(font, tmp, size, spacing).x;
      pos += copy;
    }

    if(show_caret && not has_sel && state.caret >= line_start && state.caret <= line_end) {
      float cx = text_x0 + measure_prefix_width(text, line_start, state.caret, font, size, spacing);
      if((static_cast<int>(GetTime() * 2.0) % 2) == 0) {
        DrawRectangle(static_cast<int>(cx), static_cast<int>(y), 2, static_cast<int>(size),
                      colors.caret);
      }
    }

    if(text[line_end] == '\n') {
      offset = line_end + 1;
    } else {
      break;
    }
  }
}

//----------------------------------------
//
//----------------------------------------
bool code_view_impl(Rectangle bounds, char* text, int capacity, bool can_edit, bool edit_mode,
                    CodeLanguage language, CodeViewState& state) {
  if(text != nullptr) {
    theme::ensure_ui_glyphs(text);
  }
  const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
  bool toggle = false;
  const bool keyboard = (can_edit && edit_mode) || (not can_edit && g_active_view == &state);

  const Color border = GetColor(static_cast<unsigned int>(
    GuiGetStyle(TEXTBOX, edit_mode ? BORDER_COLOR_FOCUSED : BORDER_COLOR_NORMAL)));
  const Color base = GetColor(static_cast<unsigned int>(
    GuiGetStyle(TEXTBOX, edit_mode ? BASE_COLOR_FOCUSED : BASE_COLOR_NORMAL)));

  DrawRectangleRec(bounds, base);
  DrawRectangleLinesEx(bounds, 1.0f, border);

  const Font font = GuiGetFont();
  const float size = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SIZE));
  const float spacing = static_cast<float>(GuiGetStyle(DEFAULT, TEXT_SPACING));
  const float lh = line_height();
  const int lines = count_lines(text);
  const float gutter_w = gutter_width_for(lines, font, size, spacing);

  const float content_h = measure_content_height(text);
  const Rectangle content{0.0f, 0.0f, bounds.width - 16.0f,
                          content_h < bounds.height ? bounds.height : content_h};
  Rectangle view{};
  GuiScrollPanel(bounds, nullptr, content, &state.scroll, &view);

  const auto tokens = lex_code(text, language);
  const int len = text_len(text);
  state.caret = clamp_caret(state.caret, len);
  if(state.sel_anchor >= 0) {
    state.sel_anchor = clamp_caret(state.sel_anchor, len);
  }

  const float text_x0 = view.x + state.scroll.x + gutter_w + kPad;
  const float text_y0 = view.y + state.scroll.y + kPad;

  if(hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    g_active_view = &state;
    if(can_edit) {
      toggle = true;
    }
    const Vector2 mouse = GetMousePosition();
    if(mouse.x >= view.x + gutter_w) {
      const int clicked = caret_from_click(text, font, size, spacing, lh, text_x0, text_y0, mouse);
      if(shift_down()) {
        ensure_anchor_for_shift(state);
        state.caret = clicked;
      } else {
        state.caret = clicked;
        state.sel_anchor = clicked;
        state.dragging = true;
      }
    }
  }

  if(state.dragging) {
    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      state.caret = caret_from_click(text, font, size, spacing, lh, text_x0, text_y0, mouse);
      if(state.sel_anchor < 0) {
        state.sel_anchor = state.caret;
      }
    } else {
      state.dragging = false;
      if(state.sel_anchor == state.caret) {
        clear_selection(state);
      }
    }
  }

  BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width),
                   static_cast<int>(view.height));
  draw_gutter_and_text(view, gutter_w, state.scroll, text, tokens, state, can_edit && edit_mode);
  EndScissorMode();

  if(keyboard) {
    if(ctrl_down() && IsKeyPressed(KEY_A)) {
      state.sel_anchor = 0;
      state.caret = len;
    }
    if(ctrl_down() && IsKeyPressed(KEY_C)) {
      copy_selection_to_clipboard(text, state);
    }
    if(can_edit && edit_mode && ctrl_down() && IsKeyPressed(KEY_X)) {
      if(code_view_has_selection(state)) {
        copy_selection_to_clipboard(text, state);
        delete_selection(text, state);
      }
    }
    if(can_edit && edit_mode && ctrl_down() && IsKeyPressed(KEY_V)) {
      const char* clip = GetClipboardText();
      if(clip != nullptr) {
        insert_bytes(text, capacity, state, clip, static_cast<int>(std::strlen(clip)));
      }
    }
  }

  if(can_edit && edit_mode) {
    const bool pure_ctrl = ctrl_down() && not shift_down();

    int codepoint = GetCharPressed();
    while(codepoint > 0) {
      if(not ctrl_down() && codepoint >= 32) {
        int encoded_size = 0;
        const char* encoded = CodepointToUTF8(codepoint, &encoded_size);
        if(encoded != nullptr && encoded_size > 0) {
          char bytes[8];
          const int n = std::min(encoded_size, static_cast<int>(sizeof(bytes)));
          std::memcpy(bytes, encoded, static_cast<std::size_t>(n));
          insert_bytes(text, capacity, state, bytes, n);
        }
      }
      codepoint = GetCharPressed();
    }

    if(not pure_ctrl && (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))) {
      if(code_view_has_selection(state)) {
        delete_selection(text, state);
      }
      insert_at(text, capacity, state.caret, '\n');
      clear_selection(state);
    }
    if(not pure_ctrl && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) {
      if(code_view_has_selection(state)) {
        delete_selection(text, state);
      } else {
        delete_before(text, state.caret);
      }
    }
    if(not pure_ctrl && (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE))) {
      if(code_view_has_selection(state)) {
        delete_selection(text, state);
      } else {
        delete_after(text, state.caret);
      }
    }

    const bool extend = shift_down();
    if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
      move_caret(state, previous_codepoint_start(text, state.caret), text_len(text), extend);
    }
    if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
      move_caret(state, state.caret + codepoint_bytes_at(text, state.caret, text_len(text)),
                 text_len(text), extend);
    }
    if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
      const auto info = line_info_at(text, state.caret);
      const int next = info.line > 0 ? offset_at_line_col(text, info.line - 1, info.column) : 0;
      move_caret(state, next, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
      const auto info = line_info_at(text, state.caret);
      const int max_line = count_lines(text) - 1;
      const int next = info.line < max_line ? offset_at_line_col(text, info.line + 1, info.column)
                                            : text_len(text);
      move_caret(state, next, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_HOME)) {
      move_caret(state, line_info_at(text, state.caret).line_start, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_END)) {
      move_caret(state, line_info_at(text, state.caret).line_end, text_len(text), extend);
    }
  } else if(keyboard) {
    const bool extend = shift_down();
    if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
      move_caret(state, previous_codepoint_start(text, state.caret), text_len(text), extend);
    }
    if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
      move_caret(state, state.caret + codepoint_bytes_at(text, state.caret, text_len(text)),
                 text_len(text), extend);
    }
    if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
      const auto info = line_info_at(text, state.caret);
      const int next = info.line > 0 ? offset_at_line_col(text, info.line - 1, info.column) : 0;
      move_caret(state, next, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
      const auto info = line_info_at(text, state.caret);
      const int max_line = count_lines(text) - 1;
      const int next = info.line < max_line ? offset_at_line_col(text, info.line + 1, info.column)
                                            : text_len(text);
      move_caret(state, next, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_HOME)) {
      move_caret(state, line_info_at(text, state.caret).line_start, text_len(text), extend);
    }
    if(IsKeyPressed(KEY_END)) {
      move_caret(state, line_info_at(text, state.caret).line_end, text_len(text), extend);
    }
  }

  return toggle;
}

} // namespace

//----------------------------------------
//
//----------------------------------------
std::string_view code_view_selected_text(const char* text, const CodeViewState& state) noexcept {
  if(text == nullptr || not code_view_has_selection(state)) {
    return {};
  }
  const int begin = code_view_sel_begin(state);
  const int end = code_view_sel_end(state);
  return std::string_view{text + begin, static_cast<std::size_t>(end - begin)};
}

//----------------------------------------
//
//----------------------------------------
bool code_view(Rectangle bounds, char* text, int capacity, bool edit_mode, CodeLanguage language,
               CodeViewState& state) {
  return code_view_impl(bounds, text, capacity, true, edit_mode, language, state);
}

//----------------------------------------
//
//----------------------------------------
void code_view(Rectangle bounds, const char* text, CodeLanguage language, CodeViewState& state) {
  code_view_impl(bounds, const_cast<char*>(text), 0, false, false, language, state);
}

} // namespace requestman
