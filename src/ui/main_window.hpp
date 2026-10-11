//----------------------------------------
// File:      main_window.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#pragma once

#include "app_controller.hpp"
#include "code_language.hpp"
#include "code_view.hpp"
#include "http_request.hpp"
#include "icons.hpp"
#include "ids.hpp"
#include "raylib.h"

#include <cstdint>
#include <optional>
#include <string>

namespace requestman {

inline constexpr std::size_t kUrlCapacity = 2048;
inline constexpr std::size_t kParamsCapacity = 8192;
inline constexpr std::size_t kHeadersCapacity = 8192;
inline constexpr std::size_t kCookiesCapacity = 4096;
inline constexpr std::size_t kBodyCapacity = 65536;
inline constexpr std::size_t kAuthUserCapacity = 256;
inline constexpr std::size_t kAuthSecretCapacity = 1024;
inline constexpr std::size_t kAuthKeyNameCapacity = 128;
inline constexpr std::size_t kAuthTokenUrlCapacity = 1024;
inline constexpr std::size_t kAuthClientIdCapacity = 256;
inline constexpr std::size_t kAuthScopeCapacity = 512;
inline constexpr std::size_t kAuthAccessTokenCapacity = 2048;
inline constexpr std::size_t kUserAgentCapacity = 512;
inline constexpr std::size_t kProxyUrlCapacity = 512;
inline constexpr std::size_t kProxyUserCapacity = 256;
inline constexpr std::size_t kProxyPasswordCapacity = 256;
inline constexpr std::size_t kSearchCapacity = 128;
inline constexpr std::size_t kSavedNameCapacity = 256;
inline constexpr std::size_t kResponseTextCapacity = 262144;

enum class FocusField : unsigned {
  None = 0,
  Url,
  Params,
  Headers,
  Body,
  Cookies,
  AuthUser,
  AuthSecret,
  AuthKey,
  AuthTokenUrl,
  AuthClientId,
  AuthScope,
  AuthAccessToken,
  UserAgent,
  ProxyUrl,
  ProxyUser,
  ProxyPassword,
  Search,
  SavedName,
};

enum class SidebarTab : unsigned {
  History = 0,
  Saved,
};

enum class ResponseSection : unsigned {
  Body = 0,
  Headers,
  Cookies,
};

class MainWindow {
public:
  explicit MainWindow(AppController& controller);
  void run();

private:
  void draw();
  void prepare_glyphs();
  void send();
  void save_current_request();
  void refresh_response();
  void apply_request(const HttpRequest& request);
  [[nodiscard]] HttpRequest make_request() const;
  [[nodiscard]] bool editing(FocusField field) const noexcept {
    return focus_ == field;
  }
  void toggle_focus(FocusField field) noexcept;
  void edit_url_box(Rectangle bounds);
  void beautify_body();
  void commit_saved_rename();
  void cancel_saved_rename() noexcept;

  AppController* controller_{nullptr};
  UiIcons icons_{};
  Texture2D watermark_{};
  bool watermark_loaded_{false};

  int method_index_{0};
  bool method_dropdown_open_{false};
  char url_[kUrlCapacity]{};
  bool url_select_all_{false};
  char params_[kParamsCapacity]{};
  char headers_[kHeadersCapacity]{};
  char cookies_[kCookiesCapacity]{};
  char body_[kBodyCapacity]{};
  char auth_user_[kAuthUserCapacity]{};
  char auth_secret_[kAuthSecretCapacity]{};
  char auth_key_name_[kAuthKeyNameCapacity]{};
  char auth_token_url_[kAuthTokenUrlCapacity]{};
  char auth_client_id_[kAuthClientIdCapacity]{};
  char auth_scope_[kAuthScopeCapacity]{};
  char auth_access_token_[kAuthAccessTokenCapacity]{};
  char identity_user_agent_[kUserAgentCapacity]{};
  char proxy_url_[kProxyUrlCapacity]{};
  char proxy_user_[kProxyUserCapacity]{};
  char proxy_password_[kProxyPasswordCapacity]{};
  char search_[kSearchCapacity]{};
  int auth_type_{0};
  std::int64_t auth_expires_at_{0};
  int editor_tab_{3};
  int response_view_mode_{0};
  int body_content_type_{0};
  int browser_preset_index_{0};
  bool browser_preset_dropdown_open_{false};
  bool draw_browser_preset_dropdown_{false};
  Rectangle browser_preset_dropdown_bounds_{};
  bool apply_browser_headers_{true};
  bool follow_redirects_{true};
  bool verify_tls_{true};
  bool proxy_enabled_{false};
  bool tor_enabled_{false};
  Vector2 settings_scroll_{};
  Rectangle settings_scroll_view_{};

  FocusField focus_{FocusField::None};
  SidebarTab sidebar_tab_{SidebarTab::History};
  ResponseSection response_section_{ResponseSection::Body};

  CodeViewState body_view_{};
  CodeViewState params_view_{};
  CodeViewState headers_view_{};
  CodeViewState cookies_view_{};
  CodeViewState response_view_{};
  CodeLanguage response_language_{CodeLanguage::Plain};

  std::optional<SavedRequestId> selected_saved_id_{};
  std::optional<SavedRequestId> renaming_saved_id_{};
  char rename_name_[kSavedNameCapacity]{};
  std::optional<SavedRequestId> last_saved_click_id_{};
  double last_saved_click_time_{0.0};

  int last_status_code_{0};
  std::int64_t last_elapsed_ms_{0};
  std::size_t last_body_bytes_{0};

  std::string status_{"Ready"};
  std::string response_meta_;
  std::string response_headers_text_;
  std::string response_cookies_text_;
  char response_text_[kResponseTextCapacity]{};
};

} // namespace requestman
