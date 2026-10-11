//----------------------------------------
// File:      main_window.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#include "main_window.hpp"

#include "auth.hpp"
#include "body_format.hpp"
#include "gui_text_box.hpp"
#include "http_method.hpp"
#include "logging.hpp"
#include "raygui.h"
#include "raylib.h"
#include "theme.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
HttpMethod method_from_index(int index) {
  switch(index) {
  case 1:
    return HttpMethod::Post;
  case 2:
    return HttpMethod::Put;
  case 3:
    return HttpMethod::Patch;
  case 4:
    return HttpMethod::Delete;
  case 5:
    return HttpMethod::Head;
  case 6:
    return HttpMethod::Options;
  default:
    return HttpMethod::Get;
  }
}

//----------------------------------------
//
//----------------------------------------
int index_from_method(HttpMethod method) {
  switch(method) {
  case HttpMethod::Post:
    return 1;
  case HttpMethod::Put:
    return 2;
  case HttpMethod::Patch:
    return 3;
  case HttpMethod::Delete:
    return 4;
  case HttpMethod::Head:
    return 5;
  case HttpMethod::Options:
    return 6;
  case HttpMethod::Get:
  default:
    return 0;
  }
}

//----------------------------------------
//
//----------------------------------------
std::string_view trim_view(std::string_view value) {
  while(not value.empty()
        && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while(not value.empty()
        && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

//----------------------------------------
//
//----------------------------------------
void append_pair_lines(std::string_view text, char separator, std::vector<HttpHeader>& out) {
  std::size_t begin = 0;
  while(begin <= text.size()) {
    const auto end = text.find('\n', begin);
    const auto line = trim_view(
      text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin));
    begin = end == std::string_view::npos ? text.size() + 1 : end + 1;
    if(line.empty() || line.front() == '#')
      continue;
    const auto sep = line.find(separator);
    if(sep == std::string_view::npos)
      continue;
    auto name = std::string{trim_view(line.substr(0, sep))};
    auto value = std::string{trim_view(line.substr(sep + 1))};
    if(not name.empty())
      out.push_back({.name = std::move(name), .value = std::move(value), .enabled = true});
  }
}

//----------------------------------------
//
//----------------------------------------
void append_query_lines(std::string_view text, std::vector<HttpQueryParameter>& out) {
  std::size_t begin = 0;
  while(begin <= text.size()) {
    const auto end = text.find('\n', begin);
    const auto line = trim_view(
      text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin));
    begin = end == std::string_view::npos ? text.size() + 1 : end + 1;
    if(line.empty() || line.front() == '#')
      continue;
    const auto sep = line.find('=');
    if(sep == std::string_view::npos) {
      out.push_back({.name = std::string{line}, .value = {}, .enabled = true});
      continue;
    }
    out.push_back({.name = std::string{trim_view(line.substr(0, sep))},
                   .value = std::string{trim_view(line.substr(sep + 1))},
                   .enabled = true});
  }
}

//----------------------------------------
//
//----------------------------------------
void append_cookie_lines(std::string_view text, std::vector<HttpCookie>& out) {
  std::size_t begin = 0;
  while(begin <= text.size()) {
    const auto end = text.find('\n', begin);
    const auto line = trim_view(
      text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin));
    begin = end == std::string_view::npos ? text.size() + 1 : end + 1;
    if(line.empty() || line.front() == '#')
      continue;
    const auto sep = line.find('=');
    if(sep == std::string_view::npos)
      continue;
    out.push_back({.name = std::string{trim_view(line.substr(0, sep))},
                   .value = std::string{trim_view(line.substr(sep + 1))},
                   .enabled = true});
  }
}

//----------------------------------------
//
//----------------------------------------
void draw_panel(Rectangle bounds, Color fill, Color outline, float roundness = 0.05f,
                int segments = 8) {
  DrawRectangleRounded(bounds, roundness, segments, fill);
  DrawRectangleRoundedLinesEx(bounds, roundness, segments, 1.0f, outline);
}

//----------------------------------------
//
//----------------------------------------
void draw_method_badge(Rectangle bounds, HttpMethod method) {
  const Color c = theme::method_color(method);
  DrawRectangleRounded(bounds, 0.25f, 4, Color{c.r, c.g, c.b, 45});
  DrawRectangleRoundedLinesEx(bounds, 0.25f, 4, 1.0f, c);
  const auto label = std::string{to_string(method)};
  const float size = 13.0f;
  const Vector2 m = MeasureTextEx(GuiGetFont(), label.c_str(), size, 0.0f);
  DrawTextEx(
    GuiGetFont(), label.c_str(),
    Vector2{bounds.x + (bounds.width - m.x) * 0.5f, bounds.y + (bounds.height - m.y) * 0.5f}, size,
    0.0f, c);
}

//----------------------------------------
//
//----------------------------------------
bool icon_text_button(Rectangle bounds, const UiIcons& icons, UiIcon icon, const char* label,
                      bool primary) {
  const Color fill = primary ? theme::primary() : theme::surface_raised();
  const Color border = primary ? theme::primary() : theme::border();
  const Color fg = primary ? Color{0xF8, 0xFA, 0xFC, 0xFF} : theme::text();
  draw_panel(bounds, fill, border);
  draw_ui_icon(icons, icon, Rectangle{bounds.x + 10.0f, bounds.y + 7.0f, 18.0f, 18.0f}, fg);
  DrawTextEx(GuiGetFont(), label, Vector2{bounds.x + 34.0f, bounds.y + 7.0f}, 16.0f, 0.0f, fg);
  return CheckCollisionPointRec(GetMousePosition(), bounds)
         && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string ellipsize(std::string_view text, std::size_t max_bytes) {
  if(text.size() <= max_bytes) {
    return std::string{text};
  }
  std::size_t cut = max_bytes;
  while(cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80) {
    --cut;
  }
  return std::string{text.substr(0, cut)} + "…";
}

//----------------------------------------
//
//----------------------------------------
void query_to_text(const std::vector<HttpQueryParameter>& query, char* buf, std::size_t cap) {
  std::string text;
  for(const auto& p : query) {
    if(not text.empty())
      text.push_back('\n');
    text.append(p.name).append("=").append(p.value);
  }
  std::snprintf(buf, cap, "%s", text.c_str());
}

//----------------------------------------
//
//----------------------------------------
void headers_to_text(const std::vector<HttpHeader>& headers, char* buf, std::size_t cap) {
  std::string text;
  for(const auto& h : headers) {
    if(not text.empty())
      text.push_back('\n');
    text.append(h.name).append(": ").append(h.value);
  }
  std::snprintf(buf, cap, "%s", text.c_str());
}

//----------------------------------------
//
//----------------------------------------
void cookies_to_text(const std::vector<HttpCookie>& cookies, char* buf, std::size_t cap) {
  std::string text;
  for(const auto& c : cookies) {
    if(not text.empty())
      text.push_back('\n');
    text.append(c.name).append("=").append(c.value);
  }
  std::snprintf(buf, cap, "%s", text.c_str());
}

// socks5h asks the proxy to resolve the hostname. socks5 resolves locally, and
// curl then refuses .onion names (RFC 7686) before Tor ever sees them.
constexpr std::string_view k_tor_proxy = "socks5h://127.0.0.1:9050";
constexpr std::size_t kSidebarHistoryFetchLimit = 50;

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] bool is_builtin_tor_proxy(std::string_view url) noexcept {
  return url == k_tor_proxy || url == "socks5://127.0.0.1:9050";
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
[[nodiscard]] bool paste_shortcut() {
  return (ctrl_down() && IsKeyPressed(KEY_V)) || (shift_down() && IsKeyPressed(KEY_INSERT));
}

// Copy src into dst as a single line, dropping control characters. Stops on a
// UTF-8 boundary so the buffer never ends in a partial codepoint.

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] int copy_single_line(char* dst, int dst_cap, const char* src) {
  if(dst == nullptr || dst_cap <= 1 || src == nullptr) {
    return 0;
  }
  int written = 0;
  for(int i = 0; src[i] != '\0';) {
    const auto lead = static_cast<unsigned char>(src[i]);
    if(lead < 32) {
      ++i;
      continue;
    }
    int need = 1;
    if((lead & 0xE0) == 0xC0) {
      need = 2;
    } else if((lead & 0xF0) == 0xE0) {
      need = 3;
    } else if((lead & 0xF8) == 0xF0) {
      need = 4;
    } else if(lead >= 0x80) {
      ++i;
      continue;
    }
    if(written + need >= dst_cap) {
      break;
    }
    bool complete = true;
    for(int k = 1; k < need; ++k) {
      if(src[i + k] == '\0' || (static_cast<unsigned char>(src[i + k]) & 0xC0) != 0x80) {
        complete = false;
        break;
      }
    }
    if(not complete) {
      ++i;
      continue;
    }
    for(int k = 0; k < need; ++k) {
      dst[written++] = src[i + k];
    }
    i += need;
  }
  dst[written] = '\0';
  return written;
}

//----------------------------------------
//
//----------------------------------------
void insert_single_line_at_cursor(char* text, int cap, const char* src) {
  const int len = static_cast<int>(std::strlen(text));
  int cursor = gui_text_box_cursor();
  if(cursor < 0 || cursor > len) {
    cursor = len;
  }
  const int room = cap - 1 - len;
  if(room <= 0) {
    return;
  }
  char filtered[2048];
  const int n =
    copy_single_line(filtered, std::min(room + 1, static_cast<int>(sizeof(filtered))), src);
  if(n <= 0) {
    return;
  }
  std::memmove(text + cursor + n, text + cursor, static_cast<std::size_t>(len - cursor + 1));
  std::memcpy(text + cursor, filtered, static_cast<std::size_t>(n));
  set_gui_text_box_cursor(cursor + n);
}

} // namespace

//----------------------------------------
//
//----------------------------------------
MainWindow::MainWindow(AppController& controller) : controller_(&controller) {
  std::snprintf(url_, sizeof(url_), "%s", "https://httpbin.org/get");
  std::snprintf(auth_key_name_, sizeof(auth_key_name_), "%s", "X-Api-Key");
  const auto none = resolve_client_identity(ClientIdentity{});
  std::snprintf(identity_user_agent_, sizeof(identity_user_agent_), "%s", none.user_agent.c_str());
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::toggle_focus(FocusField field) noexcept {
  focus_ = (focus_ == field) ? FocusField::None : field;
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::edit_url_box(Rectangle bounds) {
  const bool was_editing = editing(FocusField::Url);
  if(was_editing && url_select_all_) {
    if(paste_shortcut()) {
      const char* clip = GetClipboardText();
      char filtered[2048];
      const int n =
        clip == nullptr ? 0 : copy_single_line(filtered, static_cast<int>(sizeof(filtered)), clip);
      if(n > 0) {
        std::memcpy(url_, filtered, static_cast<std::size_t>(n + 1));
        set_gui_text_box_cursor(n);
        url_select_all_ = false;
        theme::ensure_ui_glyphs(url_);
      }
    } else if(ctrl_down() && IsKeyPressed(KEY_C)) {
      SetClipboardText(url_);
    } else if(ctrl_down() && IsKeyPressed(KEY_A)) {
      // Already selected.
    } else if(IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)) {
      url_[0] = '\0';
      set_gui_text_box_cursor(0);
      url_select_all_ = false;
    } else if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_HOME)
              || IsKeyPressed(KEY_END)
              || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
                  && CheckCollisionPointRec(GetMousePosition(), bounds))) {
      url_select_all_ = false;
    } else if(not ctrl_down()) {
      int codepoint = GetCharPressed();
      if(codepoint >= 32) {
        url_[0] = '\0';
        int cursor = 0;
        while(codepoint >= 32) {
          int size = 0;
          const char* encoded = CodepointToUTF8(codepoint, &size);
          if(encoded != nullptr && size > 0 && cursor + size < static_cast<int>(sizeof(url_))) {
            char bytes[8];
            std::memcpy(bytes, encoded, static_cast<std::size_t>(size));
            std::memcpy(url_ + cursor, bytes, static_cast<std::size_t>(size));
            cursor += size;
            url_[cursor] = '\0';
          }
          codepoint = GetCharPressed();
        }
        set_gui_text_box_cursor(cursor);
        url_select_all_ = false;
        theme::ensure_ui_glyphs(url_);
      }
    }
  } else if(was_editing && paste_shortcut()) {
    const char* clip = GetClipboardText();
    if(clip != nullptr) {
      insert_single_line_at_cursor(url_, static_cast<int>(sizeof(url_)), clip);
      theme::ensure_ui_glyphs(url_);
    }
  }

  if(GuiTextBox(bounds, url_, static_cast<int>(sizeof(url_)), was_editing)) {
    toggle_focus(FocusField::Url);
    if(editing(FocusField::Url)) {
      url_select_all_ = true;
      set_gui_text_box_cursor(static_cast<int>(std::strlen(url_)));
    } else {
      url_select_all_ = false;
    }
  }

  if(editing(FocusField::Url) && url_select_all_ && url_[0] != '\0') {
    const int border = GuiGetStyle(TEXTBOX, BORDER_WIDTH);
    DrawRectangle(static_cast<int>(bounds.x) + border, static_cast<int>(bounds.y) + border,
                  static_cast<int>(bounds.width) - border * 2,
                  static_cast<int>(bounds.height) - border * 2, Color{255, 255, 255, 48});
  }
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::cancel_saved_rename() noexcept {
  renaming_saved_id_.reset();
  rename_name_[0] = '\0';
  if(focus_ == FocusField::SavedName) {
    focus_ = FocusField::None;
  }
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::commit_saved_rename() {
  if(not renaming_saved_id_) {
    return;
  }

  std::string name{rename_name_};
  while(not name.empty() && std::isspace(static_cast<unsigned char>(name.front())) != 0) {
    name.erase(name.begin());
  }

  while(not name.empty() && std::isspace(static_cast<unsigned char>(name.back())) != 0) {
    name.pop_back();
  }

  if(name.empty()) {
    status_ = "saved request name is required";
    focus_ = FocusField::SavedName;
    return;
  }

  const SavedRequestId id = *renaming_saved_id_;
  auto existing = controller_->saved_requests().get(id);
  if(not existing) {
    status_ = existing.error().full_message();
    log_error(status_);
    cancel_saved_rename();
    return;
  }

  auto updated = controller_->saved_requests().update(id, std::move(name), existing->request);
  if(not updated) {
    status_ = updated.error().full_message();
    log_error(status_);
    focus_ = FocusField::SavedName;
    return;
  }

  cancel_saved_rename();
  status_ = "Renamed";
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::beautify_body() {
  const std::string_view raw{body_};
  std::size_t first = 0;
  while(first < raw.size() && std::isspace(static_cast<unsigned char>(raw[first])) != 0) {
    ++first;
  }
  if(first >= raw.size()) {
    status_ = "Nothing to beautify";
    return;
  }

  enum class Kind { Json, Xml, None };
  Kind kind = Kind::None;
  switch(body_content_type_) {
  case 1:
    kind = Kind::Json;
    break;
  case 2:
    kind = Kind::Xml;
    break;
  default:
    if(raw[first] == '{' || raw[first] == '[') {
      kind = Kind::Json;
    } else if(raw[first] == '<') {
      kind = Kind::Xml;
    }
    break;
  }

  if(kind == Kind::None) {
    status_ = "Nothing to beautify";
    return;
  }

  const std::string formatted =
    kind == Kind::Json ? format_json_pretty(raw) : format_xml_pretty(raw);
  if(formatted.empty()) {
    status_ = "Beautify failed: empty result";
    return;
  }
  if(formatted.size() >= sizeof(body_)) {
    status_ = "Beautify failed: result too large for editor";
    return;
  }
  std::snprintf(body_, sizeof(body_), "%s", formatted.c_str());
  body_view_.caret = 0;
  body_view_.sel_anchor = -1;
  body_view_.dragging = false;
  body_view_.scroll = {};
  status_ = "Beautified";
}

//----------------------------------------
//
//----------------------------------------
HttpRequest MainWindow::make_request() const {
  HttpRequest request;
  request.method = method_from_index(method_index_);
  request.url = url_;
  request.body = body_;
  append_query_lines(params_, request.query);
  append_pair_lines(headers_, ':', request.headers);
  append_cookie_lines(cookies_, request.cookies);
  request.settings.follow_redirects = follow_redirects_;
  request.settings.tls.verify_peer = verify_tls_;
  request.settings.tls.verify_host = verify_tls_;
  if(tor_enabled_) {
    request.settings.proxy.enabled = true;
    request.settings.proxy.url = std::string{k_tor_proxy};
    request.settings.proxy.username.clear();
    request.settings.proxy.password.clear();
  } else {
    request.settings.proxy.enabled = proxy_enabled_;
    request.settings.proxy.url = proxy_url_;
    request.settings.proxy.username = proxy_user_;
    request.settings.proxy.password = proxy_password_;
  }
  request.settings.identity.preset_id = std::string{browser_preset_id_at(browser_preset_index_)};
  request.settings.identity.user_agent = identity_user_agent_;
  request.settings.identity.apply_browser_headers = apply_browser_headers_;
  request.settings.identity = resolve_client_identity(request.settings.identity);

  const char* content_type = "text/plain; charset=utf-8";
  switch(body_content_type_) {
  case 1:
    content_type = "application/json";
    break;
  case 2:
    content_type = "application/xml";
    break;
  default:
    break;
  }
  bool replaced = false;
  for(auto& header : request.headers) {
    if(not equals_ci(header.name, "content-type")) {
      continue;
    }
    header.value = content_type;
    header.enabled = true;
    replaced = true;
    break;
  }

  if(not replaced) {
    request.headers.push_back({.name = "Content-Type", .value = content_type, .enabled = true});
  }

  switch(auth_type_) {
  case 1:
    request.auth = BearerAuth{.token = auth_secret_};
    break;
  case 2:
    request.auth = BasicAuth{.username = auth_user_, .password = auth_secret_};
    break;
  case 3:
    request.auth = ApiKeyAuth{.key = auth_key_name_,
                              .value = auth_secret_,
                              .location = ApiKeyAuth::Location::Header,
                              .header_name = auth_key_name_};
    break;
  case 4:
    request.auth = OAuth2ClientCredentialsAuth{.token_url = auth_token_url_,
                                               .client_id = auth_client_id_,
                                               .client_secret = auth_secret_,
                                               .scope = auth_scope_,
                                               .access_token = auth_access_token_,
                                               .expires_at = auth_expires_at_};
    break;
  default:
    request.auth = NoAuth{};
    break;
  }
  return request;
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::apply_request(const HttpRequest& request) {
  method_index_ = index_from_method(request.method);
  std::snprintf(url_, sizeof(url_), "%s", request.url.c_str());
  url_select_all_ = false;
  std::snprintf(body_, sizeof(body_), "%s", request.body.c_str());
  query_to_text(request.query, params_, sizeof(params_));
  headers_to_text(request.headers, headers_, sizeof(headers_));
  cookies_to_text(request.cookies, cookies_, sizeof(cookies_));
  follow_redirects_ = request.settings.follow_redirects;
  verify_tls_ = request.settings.tls.verify_peer;
  if(request.settings.proxy.enabled && is_builtin_tor_proxy(request.settings.proxy.url)
     && request.settings.proxy.username.empty()) {
    tor_enabled_ = true;
    proxy_enabled_ = false;
  } else {
    tor_enabled_ = false;
    proxy_enabled_ = request.settings.proxy.enabled;
    std::snprintf(proxy_url_, sizeof(proxy_url_), "%s", request.settings.proxy.url.c_str());
    std::snprintf(proxy_user_, sizeof(proxy_user_), "%s", request.settings.proxy.username.c_str());
    std::snprintf(proxy_password_, sizeof(proxy_password_), "%s",
                  request.settings.proxy.password.c_str());
  }
  const auto identity = resolve_client_identity(request.settings.identity);
  browser_preset_index_ = browser_preset_index(identity.preset_id);
  std::snprintf(identity_user_agent_, sizeof(identity_user_agent_), "%s",
                identity.user_agent.c_str());
  apply_browser_headers_ = identity.apply_browser_headers;

  if(std::holds_alternative<BearerAuth>(request.auth)) {
    auth_type_ = 1;
    std::snprintf(auth_secret_, sizeof(auth_secret_), "%s",
                  std::get<BearerAuth>(request.auth).token.c_str());
  } else if(std::holds_alternative<BasicAuth>(request.auth)) {
    auth_type_ = 2;
    const auto& basic = std::get<BasicAuth>(request.auth);
    std::snprintf(auth_user_, sizeof(auth_user_), "%s", basic.username.c_str());
    std::snprintf(auth_secret_, sizeof(auth_secret_), "%s", basic.password.c_str());
  } else if(std::holds_alternative<ApiKeyAuth>(request.auth)) {
    auth_type_ = 3;
    const auto& key = std::get<ApiKeyAuth>(request.auth);
    std::snprintf(auth_key_name_, sizeof(auth_key_name_), "%s", key.header_name.c_str());
    std::snprintf(auth_secret_, sizeof(auth_secret_), "%s", key.value.c_str());
  } else if(std::holds_alternative<OAuth2ClientCredentialsAuth>(request.auth)) {
    auth_type_ = 4;
    const auto& oauth = std::get<OAuth2ClientCredentialsAuth>(request.auth);
    std::snprintf(auth_token_url_, sizeof(auth_token_url_), "%s", oauth.token_url.c_str());
    std::snprintf(auth_client_id_, sizeof(auth_client_id_), "%s", oauth.client_id.c_str());
    std::snprintf(auth_secret_, sizeof(auth_secret_), "%s", oauth.client_secret.c_str());
    std::snprintf(auth_scope_, sizeof(auth_scope_), "%s", oauth.scope.c_str());
    std::snprintf(auth_access_token_, sizeof(auth_access_token_), "%s", oauth.access_token.c_str());
    auth_expires_at_ = oauth.expires_at;
  } else {
    auth_type_ = 0;
  }
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::run() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1440, 900, "RequestMan");
  SetTargetFPS(60);

  icons_ = load_ui_icons();
  apply_window_icon(icons_);

  {
    std::string wm_path{GetApplicationDirectory()};
    if(not wm_path.empty() && wm_path.back() != '/' && wm_path.back() != '\\') {
      wm_path.push_back('/');
    }
    wm_path += "assets/watermark.png";
    if(FileExists(wm_path.c_str())) {
      Image image = LoadImage(wm_path.c_str());
      if(image.data != nullptr) {
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        watermark_ = LoadTextureFromImage(image);
        UnloadImage(image);
        if(watermark_.id != 0) {
          SetTextureFilter(watermark_, TEXTURE_FILTER_BILINEAR);
          watermark_loaded_ = true;
        }
      }
    }
    if(not watermark_loaded_) {
      log_warn("Failed to load assets/watermark.png");
    }
  }

  auto ui_font = theme::load_ui_font();
  theme::apply_theme(ui_font);

  while(not WindowShouldClose()) {
    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_ENTER)
       && not controller_->request_in_flight()) {
      send();
    }
    if(IsKeyPressed(KEY_ESCAPE)) {
      focus_ = FocusField::None;
      url_select_all_ = false;
      method_dropdown_open_ = false;
    }
    controller_->pump();
    refresh_response();
    BeginDrawing();
    ClearBackground(theme::page_background());
    draw();
    EndDrawing();
  }

  theme::unload_ui_font(ui_font);
  if(watermark_loaded_) {
    UnloadTexture(watermark_);
    watermark_ = {};
    watermark_loaded_ = false;
  }
  unload_ui_icons(icons_);
  CloseWindow();
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::refresh_response() {
  if(controller_->request_in_flight()) {
    status_ = "Sending…";
    return;
  }
  const auto& last = controller_->last_result();
  if(not last)
    return;

  if(last->response) {
    const auto& response = *last->response;
    last_status_code_ = response.status_code;
    last_elapsed_ms_ = response.elapsed.count();
    last_body_bytes_ = response.body.size();

    response_meta_ = std::to_string(response.status_code);
    if(not response.reason_phrase.empty())
      response_meta_ += " " + response.reason_phrase;
    response_meta_ += "  •  " + std::to_string(response.elapsed.count()) + " ms  •  "
                      + std::to_string(response.body.size()) + " bytes";
    if(response.truncated)
      response_meta_ += "  •  truncated";

    const bool pretty = response_view_mode_ == 0;
    const auto formatted =
      format_response_for_display(response, pretty, sizeof(response_text_) - 1);
    std::snprintf(response_text_, sizeof(response_text_), "%s", formatted.c_str());
    response_language_ =
      sniff_code_language(header_value(response.headers, "content-type"), formatted);

    response_headers_text_.clear();
    response_cookies_text_.clear();
    for(const auto& header : response.headers) {
      response_headers_text_.append(header.name).append(": ").append(header.value).append("\n");
      if(equals_ci(header.name, "set-cookie")) {
        response_cookies_text_.append(header.value).append("\n");
      }
    }
    if(response_cookies_text_.empty())
      response_cookies_text_ = "(no Set-Cookie headers)";
    status_ = "Completed";
  } else {
    last_status_code_ = 0;
    response_meta_ = "Transport error";
    std::snprintf(response_text_, sizeof(response_text_), "%s",
                  last->response.error().full_message().c_str());
    response_language_ = CodeLanguage::Plain;
    response_headers_text_.clear();
    response_cookies_text_.clear();
    status_ = "Failed";
    log_error(last->response.error().full_message(), last->correlation.value);
  }

  if(const auto* oauth = std::get_if<OAuth2ClientCredentialsAuth>(&last->resolved_request.auth)) {
    if(not oauth->access_token.empty()) {
      std::snprintf(auth_access_token_, sizeof(auth_access_token_), "%s",
                    oauth->access_token.c_str());
      auth_expires_at_ = oauth->expires_at;
    }
  }
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::send() {
  auto sent = controller_->send_request(make_request());
  if(not sent) {
    status_ = sent.error().full_message();
    log_error(status_);
  } else {
    status_ = "Queued " + sent->value;
  }
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::save_current_request() {
  auto request = make_request();
  if(selected_saved_id_ && selected_saved_id_->valid()) {
    auto existing = controller_->saved_requests().get(*selected_saved_id_);
    if(not existing) {
      status_ = existing.error().full_message();
      log_error(status_);
      return;
    }
    auto updated =
      controller_->saved_requests().update(*selected_saved_id_, existing->name, std::move(request));
    if(not updated) {
      status_ = updated.error().full_message();
      log_error(status_);
      return;
    }
    selected_saved_id_ = updated->id;
    sidebar_tab_ = SidebarTab::Saved;
    status_ = "Saved";
    return;
  }

  std::string name = std::string{to_string(request.method)} + " ";
  std::string_view url = request.url;
  const auto scheme = url.find("://");
  if(scheme != std::string_view::npos) {
    url.remove_prefix(scheme + 3);
  }
  const auto slash = url.find('/');
  if(slash != std::string_view::npos) {
    url.remove_prefix(slash);
  } else {
    url = "/";
  }
  if(url.size() > 40) {
    name.append(ellipsize(url, 40));
  } else {
    name.append(url);
  }

  auto created = controller_->saved_requests().create(std::move(name), std::move(request));
  if(not created) {
    status_ = created.error().full_message();
    log_error(status_);
    return;
  }
  selected_saved_id_ = created->id;
  sidebar_tab_ = SidebarTab::Saved;
  status_ = "Saved";
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::prepare_glyphs() {
  std::string blob;
  blob.reserve(8192);
  const auto add = [&](std::string_view text) {
    blob.append(text);
    blob.push_back('\n');
  };
  add(response_text_);
  add(response_headers_text_);
  add(response_cookies_text_);
  add(response_meta_);
  add(status_);
  add(url_);
  add(params_);
  add(headers_);
  add(cookies_);
  add(body_);
  add(auth_user_);
  add(auth_secret_);
  add(auth_key_name_);
  add(auth_token_url_);
  add(auth_client_id_);
  add(auth_scope_);
  add(auth_access_token_);
  add(identity_user_agent_);
  add(proxy_url_);
  add(proxy_user_);
  add(proxy_password_);
  add(search_);
  add(rename_name_);

  if(sidebar_tab_ == SidebarTab::History) {
    if(const auto entries = controller_->history().list_entries(kSidebarHistoryFetchLimit)) {
      for(const auto& entry : *entries) {
        add(entry.url);
      }
    }
  } else if(const auto saved = controller_->saved_requests().list()) {
    for(const auto& entry : *saved) {
      add(entry.name);
      add(entry.request.url);
    }
  }
  theme::ensure_ui_glyphs(blob);
}

//----------------------------------------
//
//----------------------------------------
void MainWindow::draw() {
  prepare_glyphs();
  const float width = static_cast<float>(GetScreenWidth());
  const float height = static_cast<float>(GetScreenHeight());
  constexpr float sidebar_w = 280.0f;
  constexpr float pad = 12.0f;
  draw_browser_preset_dropdown_ = false;

  DrawRectangle(0, 0, static_cast<int>(sidebar_w), static_cast<int>(height), theme::surface());
  DrawLine(static_cast<int>(sidebar_w), 0, static_cast<int>(sidebar_w), static_cast<int>(height),
           theme::border());

  draw_ui_icon(icons_, UiIcon::LogoMark, Rectangle{14, 12, 28, 28}, WHITE);
  DrawTextEx(GuiGetFont(), "Request", Vector2{50, 14}, 20.0f, 0.0f, theme::text());
  const Vector2 req_sz = MeasureTextEx(GuiGetFont(), "Request", 20.0f, 0.0f);
  DrawTextEx(GuiGetFont(), "Man", Vector2{50 + req_sz.x, 14}, 20.0f, 0.0f, theme::primary_light());

  {
    const Rectangle history_tab{12, 50, (sidebar_w - 28) * 0.5f, 28};
    const Rectangle saved_tab{history_tab.x + history_tab.width + 4, 50, history_tab.width, 28};
    const bool history_active = sidebar_tab_ == SidebarTab::History;
    const bool saved_active = sidebar_tab_ == SidebarTab::Saved;
    if(history_active)
      DrawRectangleRounded(history_tab, 0.15f, 6, Color{0x25, 0x63, 0xEB, 0x40});
    if(saved_active)
      DrawRectangleRounded(saved_tab, 0.15f, 6, Color{0x25, 0x63, 0xEB, 0x40});
    draw_ui_icon(icons_, UiIcon::History, Rectangle{history_tab.x + 8, history_tab.y + 5, 16, 16},
                 history_active ? theme::primary_light() : theme::text_muted());
    DrawTextEx(GuiGetFont(), "History", Vector2{history_tab.x + 28, history_tab.y + 6}, 14.0f, 0.0f,
               history_active ? theme::text() : theme::text_muted());
    draw_ui_icon(icons_, UiIcon::Save, Rectangle{saved_tab.x + 8, saved_tab.y + 5, 16, 16},
                 saved_active ? theme::primary_light() : theme::text_muted());
    DrawTextEx(GuiGetFont(), "Saved", Vector2{saved_tab.x + 28, saved_tab.y + 6}, 14.0f, 0.0f,
               saved_active ? theme::text() : theme::text_muted());
    if(CheckCollisionPointRec(GetMousePosition(), history_tab)
       && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      if(sidebar_tab_ != SidebarTab::History) {
        cancel_saved_rename();
      }
      sidebar_tab_ = SidebarTab::History;
    }
    if(CheckCollisionPointRec(GetMousePosition(), saved_tab)
       && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      sidebar_tab_ = SidebarTab::Saved;
    }
  }

  draw_panel(Rectangle{12, 82, sidebar_w - 24, 30}, theme::page_background(), theme::border());
  draw_ui_icon(icons_, UiIcon::Search, Rectangle{18, 89, 16, 16}, theme::text_muted());
  if(GuiTextBox(Rectangle{40, 82, sidebar_w - 56, 30}, search_, static_cast<int>(sizeof(search_)),
                editing(FocusField::Search))) {
    toggle_focus(FocusField::Search);
  }

  // Reserve footer for watermark caption; image overlaps the list bottom.
  constexpr float wm_display_w = 256.0f;
  constexpr float wm_display_h = wm_display_w * (256.0f / 1008.0f);
  constexpr float wm_overlap = 28.0f;
  constexpr float footer_text_h = 92.0f;
  const float footer_reserve = footer_text_h + (wm_display_h - wm_overlap);
  const Rectangle list{8, 122, sidebar_w - 16, height - 122.0f - footer_reserve};
  float y = list.y;
  const std::string_view needle{search_};

  if(sidebar_tab_ == SidebarTab::History) {
    if(auto entries = controller_->history().list_entries(kSidebarHistoryFetchLimit)) {
      for(const auto& entry : *entries) {
        if(y > list.y + list.height - 28)
          break;
        if(not contains_ci(entry.url, needle))
          continue;
        const Rectangle row{12, y, sidebar_w - 24, 28};
        const Rectangle trash{row.x + row.width - 26, row.y + 4, 20, 20};
        draw_method_badge({row.x + 6, row.y + 5, 48, 18}, entry.method);
        const std::string label = ellipsize(entry.url, 22);
        DrawTextEx(GuiGetFont(), label.c_str(), Vector2{row.x + 60, row.y + 6}, 13.0f, 0.0f,
                   theme::text());
        const bool hover_trash = CheckCollisionPointRec(GetMousePosition(), trash);
        draw_ui_icon(icons_, UiIcon::Trash, trash,
                     hover_trash ? theme::danger() : theme::text_muted());
        if(hover_trash && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if(auto removed = controller_->history().remove(entry.id); not removed) {
            status_ = removed.error().full_message();
            log_error(status_);
          } else {
            status_ = "History entry deleted";
          }
        } else if(CheckCollisionPointRec(GetMousePosition(), row)
                  && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          selected_saved_id_.reset();
          apply_request(entry.request);
        }
        y += 30.0f;
      }
    }
  } else if(auto saved = controller_->saved_requests().list()) {
    for(const auto& entry : *saved) {
      if(y > list.y + list.height - 28)
        break;
      if(not contains_ci(entry.name, needle) && not contains_ci(entry.request.url, needle))
        continue;
      const Rectangle row{12, y, sidebar_w - 24, 28};
      const Rectangle trash{row.x + row.width - 26, row.y + 4, 20, 20};
      const Rectangle duplicate{row.x + row.width - 52, row.y + 4, 20, 20};
      const bool renaming = renaming_saved_id_ && *renaming_saved_id_ == entry.id;
      const bool selected = selected_saved_id_ && *selected_saved_id_ == entry.id;
      if(selected)
        DrawRectangleRounded(row, 0.15f, 6, Color{0x25, 0x63, 0xEB, 0x40});
      draw_method_badge({row.x + 6, row.y + 5, 48, 18}, entry.request.method);

      if(renaming) {
        if(editing(FocusField::SavedName)) {
          if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            commit_saved_rename();
          } else if(IsKeyPressed(KEY_ESCAPE)) {
            cancel_saved_rename();
          }
        }
        if(renaming_saved_id_ && *renaming_saved_id_ == entry.id) {
          const Rectangle name_box{row.x + 58, row.y + 2, row.width - 64, 24};
          if(GuiTextBox(name_box, rename_name_, static_cast<int>(sizeof(rename_name_)),
                        editing(FocusField::SavedName))) {
            if(editing(FocusField::SavedName)) {
              commit_saved_rename();
            } else {
              focus_ = FocusField::SavedName;
            }
          }
        } else {
          const std::string label = ellipsize(entry.name, 18);
          DrawTextEx(GuiGetFont(), label.c_str(), Vector2{row.x + 60, row.y + 6}, 13.0f, 0.0f,
                     theme::text());
        }
      } else {
        const std::string label = ellipsize(entry.name, 18);
        DrawTextEx(GuiGetFont(), label.c_str(), Vector2{row.x + 60, row.y + 6}, 13.0f, 0.0f,
                   theme::text());
        const bool hover_duplicate = CheckCollisionPointRec(GetMousePosition(), duplicate);
        draw_ui_icon(icons_, UiIcon::Copy, duplicate,
                     hover_duplicate ? theme::primary_light() : theme::text_muted());
        const bool hover_trash = CheckCollisionPointRec(GetMousePosition(), trash);
        draw_ui_icon(icons_, UiIcon::Trash, trash,
                     hover_trash ? theme::danger() : theme::text_muted());

        const auto finish_other_rename = [this]() {
          if(not renaming_saved_id_) {
            return true;
          }
          commit_saved_rename();
          return not renaming_saved_id_;
        };

        if(hover_duplicate && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if(finish_other_rename()) {
            std::string dup_name = entry.name;
            dup_name.append(" (copy)");
            if(auto created =
                 controller_->saved_requests().create(std::move(dup_name), entry.request);
               not created) {
              status_ = created.error().full_message();
              log_error(status_);
            } else {
              selected_saved_id_ = created->id;
              apply_request(created->request);
              status_ = "Request duplicated";
            }
          }
        } else if(hover_trash && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if(finish_other_rename()) {
            if(auto removed = controller_->saved_requests().remove(entry.id); not removed) {
              status_ = removed.error().full_message();
              log_error(status_);
            } else {
              if(selected_saved_id_ && *selected_saved_id_ == entry.id) {
                selected_saved_id_.reset();
              }
              status_ = "Saved request deleted";
            }
          }
        } else if(CheckCollisionPointRec(GetMousePosition(), row)
                  && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if(finish_other_rename()) {
            const double now = GetTime();
            const bool is_double = last_saved_click_id_ && *last_saved_click_id_ == entry.id
                                   && (now - last_saved_click_time_) <= 0.4;
            last_saved_click_id_ = entry.id;
            last_saved_click_time_ = now;
            selected_saved_id_ = entry.id;
            apply_request(entry.request);
            if(is_double) {
              renaming_saved_id_ = entry.id;
              std::snprintf(rename_name_, sizeof(rename_name_), "%s", entry.name.c_str());
              focus_ = FocusField::SavedName;
            }
          }
        }
      }
      y += 30.0f;
    }
    if(saved->empty()) {
      DrawTextEx(GuiGetFont(), "No saved requests", Vector2{20, y + 8}, 14.0f, 0.0f,
                 theme::text_muted());
    }
  }

  {
    const float wm_x = 12.0f;
    const float wm_y = list.y + list.height - wm_overlap;
    if(watermark_loaded_) {
      const Rectangle src{0.0f, 0.0f, static_cast<float>(watermark_.width),
                          static_cast<float>(watermark_.height)};
      const Rectangle dst{wm_x, wm_y, wm_display_w, wm_display_h};
      BeginBlendMode(BLEND_ALPHA);
      DrawTexturePro(watermark_, src, dst, Vector2{0.0f, 0.0f}, 0.0f, Color{255, 255, 255, 200});
      EndBlendMode();
    }

    const float text_x = 12.0f;
    float text_y = wm_y + wm_display_h + 4.0f;
    const float text_w = sidebar_w - 24.0f;
    const float font_size = 13.0f;
    const Color muted = theme::text_muted();
#ifndef REQUESTMAN_VERSION
#define REQUESTMAN_VERSION "0.1.0"
#endif
    const char* version_line = "RequestMan v" REQUESTMAN_VERSION;
    DrawTextEx(GuiGetFont(), version_line, Vector2{text_x, text_y}, font_size, 0.0f, muted);
    text_y += 16.0f;
    DrawTextEx(GuiGetFont(), "(c) Erarnitox - donation ware", Vector2{text_x, text_y}, font_size,
               0.0f, muted);
    text_y += 16.0f;
    DrawTextEx(GuiGetFont(), "BTC donations appreciated:", Vector2{text_x, text_y}, font_size, 0.0f,
               muted);
    text_y += 16.0f;
    const char* btc = "1DAviD9V2BhRrzMuYaJByUf1Mw55ctNsiV";
    constexpr float copy_size = 16.0f;
    const Rectangle btc_copy{sidebar_w - 12.0f - copy_size, text_y, copy_size, copy_size};
    const float addr_w = text_w - copy_size - 6.0f;
    // Wrap address if needed for narrow sidebar.
    const Vector2 btc_sz = MeasureTextEx(GuiGetFont(), btc, font_size, 0.0f);
    if(btc_sz.x <= addr_w) {
      DrawTextEx(GuiGetFont(), btc, Vector2{text_x, text_y}, font_size, 0.0f, muted);
    } else {
      const std::size_t mid = 17;
      const std::string line1{btc, mid};
      const std::string line2{btc + mid};
      DrawTextEx(GuiGetFont(), line1.c_str(), Vector2{text_x, text_y}, font_size, 0.0f, muted);
      text_y += 15.0f;
      DrawTextEx(GuiGetFont(), line2.c_str(), Vector2{text_x, text_y}, font_size, 0.0f, muted);
    }
    const bool hover_btc_copy = CheckCollisionPointRec(GetMousePosition(), btc_copy);
    draw_ui_icon(icons_, UiIcon::Copy, btc_copy,
                 hover_btc_copy ? theme::primary_light() : theme::text_muted());
    if(hover_btc_copy && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      SetClipboardText(btc);
      status_ = "Copied BTC address";
    }
  }

  const float main_x = sidebar_w + pad;
  const float main_w = width - main_x - pad;
  const float req_h = (height - pad * 3.0f) * 0.52f;
  const Rectangle request_panel{main_x, pad, main_w, req_h};
  draw_panel(request_panel, theme::surface(), theme::border(), 0.015f, 4);

  const Rectangle method_bounds{main_x + 12, pad + 14, 110, 32};
  const float send_w = 110.0f;
  const float save_w = 100.0f;
  const Rectangle url_bounds{main_x + 130, pad + 14, main_w - 130 - send_w - save_w - 36, 32};
  edit_url_box(url_bounds);
  if(icon_text_button({main_x + main_w - send_w - save_w - 24, pad + 14, send_w, 32}, icons_,
                      UiIcon::Send, controller_->request_in_flight() ? "…" : "Send", true)
     && not controller_->request_in_flight()) {
    send();
  }
  if(icon_text_button({main_x + main_w - save_w - 12, pad + 14, save_w, 32}, icons_, UiIcon::Save,
                      "Save", false)) {
    save_current_request();
  }

  const char* tabs[] = {"Params", "Auth", "Headers", "Body", "Cookies", "Settings"};
  float tab_x = main_x + 12;
  for(int i = 0; i < 6; ++i) {
    const Vector2 ts = MeasureTextEx(GuiGetFont(), tabs[i], 15.0f, 0.0f);
    const Rectangle tab{tab_x, pad + 56, ts.x + 18, 26};
    const bool active = editor_tab_ == i;
    DrawTextEx(GuiGetFont(), tabs[i], Vector2{tab.x + 8, tab.y + 4}, 15.0f, 0.0f,
               active ? theme::text() : theme::text_muted());
    if(active) {
      DrawRectangle(static_cast<int>(tab.x + 6), static_cast<int>(tab.y + 24),
                    static_cast<int>(ts.x + 4), 2, theme::primary());
    }
    if(CheckCollisionPointRec(GetMousePosition(), tab)
       && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      if(editor_tab_ == 5 && i != 5) {
        settings_scroll_ = {};
        browser_preset_dropdown_open_ = false;
      }
      editor_tab_ = i;
    }
    tab_x += ts.x + 26;
  }

  const Rectangle content{main_x + 12, pad + 90, main_w - 24, req_h - 102};
  if(editor_tab_ == 0) {
    GuiLabel({content.x, content.y, content.width, 20}, "One query param per line: name=value");
    const Rectangle params_bounds{content.x, content.y + 24, content.width, content.height - 24};
    if(code_view(params_bounds, params_, static_cast<int>(sizeof(params_)),
                 editing(FocusField::Params), CodeLanguage::Plain, params_view_)) {
      toggle_focus(FocusField::Params);
    }
  } else if(editor_tab_ == 1) {
    (void)GuiToggleGroup({content.x, content.y, 72, 28}, "None;Bearer;Basic;API Key;OAuth2",
                         &auth_type_);
    if(auth_type_ == 2) {
      if(GuiTextBox({content.x, content.y + 40, content.width, 28}, auth_user_,
                    static_cast<int>(sizeof(auth_user_)), editing(FocusField::AuthUser))) {
        toggle_focus(FocusField::AuthUser);
      }
      GuiLabel({content.x, content.y + 72, content.width, 20}, "Username above, password below");
      if(GuiTextBox({content.x, content.y + 96, content.width, 28}, auth_secret_,
                    static_cast<int>(sizeof(auth_secret_)), editing(FocusField::AuthSecret))) {
        toggle_focus(FocusField::AuthSecret);
      }
    } else if(auth_type_ == 3) {
      if(GuiTextBox({content.x, content.y + 40, content.width, 28}, auth_key_name_,
                    static_cast<int>(sizeof(auth_key_name_)), editing(FocusField::AuthKey))) {
        toggle_focus(FocusField::AuthKey);
      }
      GuiLabel({content.x, content.y + 72, content.width, 20}, "Header name above, value below");
      if(GuiTextBox({content.x, content.y + 96, content.width, 28}, auth_secret_,
                    static_cast<int>(sizeof(auth_secret_)), editing(FocusField::AuthSecret))) {
        toggle_focus(FocusField::AuthSecret);
      }
    } else if(auth_type_ == 1) {
      GuiLabel({content.x, content.y + 40, content.width, 20}, "Bearer token");
      if(GuiTextBox({content.x, content.y + 64, content.width, 28}, auth_secret_,
                    static_cast<int>(sizeof(auth_secret_)), editing(FocusField::AuthSecret))) {
        toggle_focus(FocusField::AuthSecret);
      }
    } else if(auth_type_ == 4) {
      GuiLabel({content.x, content.y + 36, content.width, 18}, "Token URL");
      if(GuiTextBox({content.x, content.y + 54, content.width, 26}, auth_token_url_,
                    static_cast<int>(sizeof(auth_token_url_)), editing(FocusField::AuthTokenUrl))) {
        toggle_focus(FocusField::AuthTokenUrl);
      }
      GuiLabel({content.x, content.y + 84, content.width * 0.5f - 4, 18}, "Client ID");
      GuiLabel({content.x + content.width * 0.5f + 4, content.y + 84, content.width * 0.5f - 4, 18},
               "Client Secret");
      if(GuiTextBox({content.x, content.y + 102, content.width * 0.5f - 4, 26}, auth_client_id_,
                    static_cast<int>(sizeof(auth_client_id_)), editing(FocusField::AuthClientId))) {
        toggle_focus(FocusField::AuthClientId);
      }
      if(GuiTextBox(
           {content.x + content.width * 0.5f + 4, content.y + 102, content.width * 0.5f - 4, 26},
           auth_secret_, static_cast<int>(sizeof(auth_secret_)), editing(FocusField::AuthSecret))) {
        toggle_focus(FocusField::AuthSecret);
      }
      GuiLabel({content.x, content.y + 132, content.width, 18}, "Scope (optional)");
      if(GuiTextBox({content.x, content.y + 150, content.width, 26}, auth_scope_,
                    static_cast<int>(sizeof(auth_scope_)), editing(FocusField::AuthScope))) {
        toggle_focus(FocusField::AuthScope);
      }
      GuiLabel({content.x, content.y + 180, content.width, 18}, "Access Token");
      if(GuiTextBox({content.x, content.y + 198, content.width, 26}, auth_access_token_,
                    static_cast<int>(sizeof(auth_access_token_)),
                    editing(FocusField::AuthAccessToken))) {
        toggle_focus(FocusField::AuthAccessToken);
      }
      GuiLabel({content.x, content.y + 230, content.width, 20},
               "Token will be fetched on Send if missing/expired");
    }
  } else if(editor_tab_ == 2) {
    GuiLabel({content.x, content.y, content.width, 20}, "One header per line: Name: value");
    const Rectangle headers_bounds{content.x, content.y + 24, content.width, content.height - 24};
    if(code_view(headers_bounds, headers_, static_cast<int>(sizeof(headers_)),
                 editing(FocusField::Headers), CodeLanguage::Plain, headers_view_)) {
      toggle_focus(FocusField::Headers);
    }
  } else if(editor_tab_ == 3) {
    GuiLabel({content.x, content.y, 100, 24}, "Content-Type");
    (void)GuiToggleGroup({content.x + 110, content.y, 80, 26}, "Plain;JSON;XML",
                         &body_content_type_);
    const Rectangle beautify{content.x + content.width - 110, content.y, 110, 26};
    draw_ui_icon(icons_, UiIcon::Beautify, Rectangle{beautify.x, beautify.y + 4, 18, 18},
                 theme::text_muted());
    DrawTextEx(GuiGetFont(), "Beautify", Vector2{beautify.x + 24, beautify.y + 5}, 14.0f, 0.0f,
               theme::text_muted());
    if(CheckCollisionPointRec(GetMousePosition(), beautify)
       && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      beautify_body();
    }
    const Rectangle body_bounds{content.x, content.y + 34, content.width, content.height - 34};
    if(code_view(body_bounds, body_, static_cast<int>(sizeof(body_)), editing(FocusField::Body),
                 code_language_from_body_content_type(body_content_type_), body_view_)) {
      toggle_focus(FocusField::Body);
    }
  } else if(editor_tab_ == 4) {
    GuiLabel({content.x, content.y, content.width, 20}, "One cookie per line: name=value");
    const Rectangle cookie_bounds{content.x, content.y + 24, content.width, content.height - 24};
    if(code_view(cookie_bounds, cookies_, static_cast<int>(sizeof(cookies_)),
                 editing(FocusField::Cookies), CodeLanguage::Plain, cookies_view_)) {
      toggle_focus(FocusField::Cookies);
    }
  } else {
    constexpr float settings_pad = 8.0f;
    const float form_h = (proxy_enabled_ ? 460.0f : 280.0f) + 24.0f + settings_pad;
    const Rectangle scroll_content{0.0f, 0.0f, content.width - 16.0f, form_h};
    Rectangle view{};
    GuiScrollPanel(content, nullptr, scroll_content, &settings_scroll_, &view);
    settings_scroll_view_ = view;

    const float ox = view.x + settings_scroll_.x + settings_pad;
    const float oy = view.y + settings_scroll_.y + settings_pad;
    const float form_w = scroll_content.width - settings_pad;

    browser_preset_dropdown_bounds_ = {ox + 160.0f, oy + 80.0f, form_w - 160.0f, 28.0f};
    // Preset dropdown is drawn at end of frame (after response panel) so the open
    // list paints above User-Agent / buttons / fingerprint text.
    draw_browser_preset_dropdown_ =
      CheckCollisionRecs(browser_preset_dropdown_bounds_, view) || browser_preset_dropdown_open_;

    BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y),
                     static_cast<int>(view.width), static_cast<int>(view.height));

    if(browser_preset_dropdown_open_) {
      GuiLock();
    }
    if(GuiButton({ox, oy, 280, 28},
                 follow_redirects_ ? "Follow redirects: ON" : "Follow redirects: OFF")) {
      follow_redirects_ = not follow_redirects_;
    }
    if(GuiButton({ox, oy + 40, 280, 28}, verify_tls_ ? "TLS verify: ON" : "TLS verify: OFF")) {
      verify_tls_ = not verify_tls_;
    }

    GuiLabel({ox, oy + 84, 160, 20}, "Browser Preset");

    GuiLabel({ox, oy + 120, form_w, 20}, "User-Agent");
    if(GuiTextBox({ox, oy + 142, form_w, 28}, identity_user_agent_,
                  static_cast<int>(sizeof(identity_user_agent_)), editing(FocusField::UserAgent))) {
      toggle_focus(FocusField::UserAgent);
    }
    if(GuiButton({ox, oy + 182, 320, 28}, apply_browser_headers_
                                            ? "Apply browser default headers: ON"
                                            : "Apply browser default headers: OFF")) {
      apply_browser_headers_ = not apply_browser_headers_;
    }
    const auto* preset = find_browser_preset(browser_preset_id_at(browser_preset_index_));
    const std::string fingerprint = (preset != nullptr && not preset->impersonate_target.empty())
                                      ? std::string{preset->impersonate_target}
                                      : std::string{"none"};
    GuiLabel({ox, oy + 220, form_w, 20}, TextFormat("TLS/H2 fingerprint: %s", fingerprint.c_str()));
    GuiLabel({ox, oy + 244, form_w, 20},
             "Timeout 30s • Max response 32 MiB • Protocols http/https only");

    if(tor_enabled_) {
      GuiDisable();
    }
    if(GuiButton({ox, oy + 276, 136, 28}, proxy_enabled_ ? "Proxy: ON" : "Proxy: OFF")) {
      proxy_enabled_ = not proxy_enabled_;
      if(proxy_enabled_) {
        tor_enabled_ = false;
      }
    }
    if(tor_enabled_) {
      GuiEnable();
    }
    if(GuiButton({ox + 144, oy + 276, 136, 28}, tor_enabled_ ? "Use Tor: ON" : "Use Tor")) {
      tor_enabled_ = not tor_enabled_;
      if(tor_enabled_) {
        proxy_enabled_ = false;
        if(focus_ == FocusField::ProxyUrl || focus_ == FocusField::ProxyUser
           || focus_ == FocusField::ProxyPassword) {
          focus_ = FocusField::None;
        }
      }
    }
    if(tor_enabled_) {
      GuiLabel({ox, oy + 312, form_w, 20}, "Tor proxy: socks5h://127.0.0.1:9050");
    }
    if(proxy_enabled_) {
      GuiLabel({ox, oy + 316, form_w, 20}, "Proxy URL (http://host:port or socks5://host:port)");
      if(GuiTextBox({ox, oy + 338, form_w, 28}, proxy_url_, static_cast<int>(sizeof(proxy_url_)),
                    editing(FocusField::ProxyUrl))) {
        toggle_focus(FocusField::ProxyUrl);
      }
      GuiLabel({ox, oy + 378, 120, 20}, "Username");
      if(GuiTextBox({ox + 120, oy + 374, form_w - 120, 28}, proxy_user_,
                    static_cast<int>(sizeof(proxy_user_)), editing(FocusField::ProxyUser))) {
        toggle_focus(FocusField::ProxyUser);
      }
      GuiLabel({ox, oy + 418, 120, 20}, "Password");
      if(GuiTextBox({ox + 120, oy + 414, form_w - 120, 28}, proxy_password_,
                    static_cast<int>(sizeof(proxy_password_)),
                    editing(FocusField::ProxyPassword))) {
        toggle_focus(FocusField::ProxyPassword);
      }
    }
    if(browser_preset_dropdown_open_) {
      GuiUnlock();
    }
    EndScissorMode();
  }

  const float resp_y = pad + req_h + pad;
  const float resp_h = height - resp_y - pad;
  const Rectangle response_panel{main_x, resp_y, main_w, resp_h};
  draw_panel(response_panel, theme::surface(), theme::border(), 0.015f, 4);

  const char* resp_tabs[] = {"Response", "Headers", "Cookies"};
  if(static_cast<unsigned>(response_section_) > static_cast<unsigned>(ResponseSection::Cookies)) {
    response_section_ = ResponseSection::Body;
  }
  float rt_x = main_x + 12;
  for(int i = 0; i < 3; ++i) {
    const Vector2 ts = MeasureTextEx(GuiGetFont(), resp_tabs[i], 15.0f, 0.0f);
    const Rectangle tab{rt_x, resp_y + 10, ts.x + 16, 26};
    const bool active = static_cast<int>(response_section_) == i;
    DrawTextEx(GuiGetFont(), resp_tabs[i], Vector2{tab.x + 6, tab.y + 4}, 15.0f, 0.0f,
               active ? theme::text() : theme::text_muted());
    if(active) {
      DrawRectangle(static_cast<int>(tab.x + 4), static_cast<int>(tab.y + 24),
                    static_cast<int>(ts.x + 4), 2, theme::primary());
    }
    if(CheckCollisionPointRec(GetMousePosition(), tab)
       && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      if(response_section_ != static_cast<ResponseSection>(i)) {
        response_view_.sel_anchor = -1;
        response_view_.dragging = false;
        response_view_.caret = 0;
        response_view_.scroll = {};
      }
      response_section_ = static_cast<ResponseSection>(i);
    }
    rt_x += ts.x + 24;
  }

  if(last_status_code_ > 0) {
    char meta[160];
    std::snprintf(meta, sizeof(meta), "%d  •  %lld ms  •  %zu B", last_status_code_,
                  static_cast<long long>(last_elapsed_ms_), last_body_bytes_);
    const Vector2 ms = MeasureTextEx(GuiGetFont(), meta, 14.0f, 0.0f);
    DrawTextEx(GuiGetFont(), meta, Vector2{main_x + main_w - ms.x - 16, resp_y + 14}, 14.0f, 0.0f,
               theme::status_color(last_status_code_));
  } else {
    const Vector2 ms = MeasureTextEx(GuiGetFont(), status_.c_str(), 14.0f, 0.0f);
    DrawTextEx(GuiGetFont(), status_.c_str(), Vector2{main_x + main_w - ms.x - 16, resp_y + 14},
               14.0f, 0.0f, theme::text_muted());
  }

  (void)GuiToggleGroup({main_x + 12, resp_y + 44, 80, 24}, "Pretty;Raw", &response_view_mode_);
  if(response_view_mode_ > 1)
    response_view_mode_ = 0;
  const char* response_section_text = response_text_;
  if(response_section_ == ResponseSection::Headers) {
    response_section_text = response_headers_text_.c_str();
  } else if(response_section_ == ResponseSection::Cookies) {
    response_section_text = response_cookies_text_.c_str();
  }

  const Rectangle copy_btn{main_x + main_w - 40, resp_y + 44, 24, 24};
  draw_ui_icon(icons_, UiIcon::Copy, copy_btn, theme::text_muted());
  if(CheckCollisionPointRec(GetMousePosition(), copy_btn)
     && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    if(const auto selected = code_view_selected_text(response_section_text, response_view_);
       not selected.empty()) {
      const std::string slice{selected};
      SetClipboardText(slice.c_str());
      status_ = "Copied selection";
    } else {
      SetClipboardText(response_section_text);
      status_ = "Copied response";
    }
  }

  const Rectangle response_bounds{main_x + 12, resp_y + 78, main_w - 24, resp_h - 90};
  if(response_section_ == ResponseSection::Body) {
    code_view(response_bounds, response_text_, response_language_, response_view_);
  } else if(response_section_ == ResponseSection::Headers) {
    code_view(response_bounds, response_headers_text_.c_str(), CodeLanguage::Plain, response_view_);
  } else if(response_section_ == ResponseSection::Cookies) {
    code_view(response_bounds, response_cookies_text_.c_str(), CodeLanguage::Plain, response_view_);
  }

  // Browser preset dropdown after response chrome so the open list is not covered.
  if(draw_browser_preset_dropdown_) {
    const std::string preset_labels = browser_preset_dropdown_labels();
    if(GuiDropdownBox(browser_preset_dropdown_bounds_, preset_labels.c_str(),
                      &browser_preset_index_, browser_preset_dropdown_open_)) {
      browser_preset_dropdown_open_ = not browser_preset_dropdown_open_;
      if(not browser_preset_dropdown_open_) {
        ClientIdentity identity;
        identity.preset_id = std::string{browser_preset_id_at(browser_preset_index_)};
        identity = resolve_client_identity(std::move(identity));
        std::snprintf(identity_user_agent_, sizeof(identity_user_agent_), "%s",
                      identity.user_agent.c_str());
      }
    }
  }

  // Method dropdown drawn last so its open list paints above neighboring controls.
  const Color method_col = theme::method_color(method_from_index(method_index_));
  GuiSetStyle(DROPDOWNBOX, BORDER_COLOR_NORMAL, ColorToInt(method_col));
  GuiSetStyle(DROPDOWNBOX, TEXT_COLOR_NORMAL, ColorToInt(method_col));
  if(GuiDropdownBox(method_bounds, "GET;POST;PUT;PATCH;DELETE;HEAD;OPTIONS", &method_index_,
                    method_dropdown_open_)) {
    method_dropdown_open_ = not method_dropdown_open_;
  }
}

} // namespace requestman
