//----------------------------------------
// File:      browser_presets.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "browser_presets.hpp"

#include <algorithm>

namespace requestman {
namespace {

// Pinned curl-impersonate targets (lexiforest v2.2.2). Bump in one place when fingerprints go stale.
constexpr std::string_view kChromeTarget = "chrome146";
constexpr std::string_view kFirefoxTarget = "firefox147";
constexpr std::string_view kSafariTarget = "safari26_0";

// Chrome TLS target + Edge UA/hints (edge101 fingerprint is far behind current Edge).
constexpr std::string_view kEdgeTlsTarget = "chrome146";

constexpr std::string_view kChromeWinUa =
  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
  "Chrome/146.0.0.0 Safari/537.36";
constexpr std::string_view kChromeMacUa =
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) "
  "Chrome/146.0.0.0 Safari/537.36";
constexpr std::string_view kEdgeWinUa =
  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
  "Chrome/146.0.0.0 Safari/537.36 Edg/146.0.0.0";
constexpr std::string_view kFirefoxWinUa =
  "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:147.0) Gecko/20100101 Firefox/147.0";
constexpr std::string_view kSafariMacUa =
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) "
  "Version/26.0 Safari/605.1.15";
constexpr std::string_view kRequestManUa = "RequestMan/0.1";

} // namespace

//----------------------------------------
//
//----------------------------------------
const std::vector<BrowserPreset>& browser_presets() {
  static const std::vector<BrowserPreset> presets{
    {.id = "none",
     .label = "None (RequestMan)",
     .impersonate_target = "",
     .default_user_agent = kRequestManUa,
     .sec_ch_ua_platform = ""},
    {.id = "chrome_win11",
     .label = "Google Chrome Windows 11",
     .impersonate_target = kChromeTarget,
     .default_user_agent = kChromeWinUa,
     .sec_ch_ua_platform = "\"Windows\""},
    {.id = "chrome_macos",
     .label = "Google Chrome macOS",
     .impersonate_target = kChromeTarget,
     .default_user_agent = kChromeMacUa,
     .sec_ch_ua_platform = "\"macOS\""},
    {.id = "edge_win11",
     .label = "Microsoft Edge Windows 11",
     .impersonate_target = kEdgeTlsTarget,
     .default_user_agent = kEdgeWinUa,
     .sec_ch_ua_platform = "\"Windows\""},
    {.id = "firefox_win11",
     .label = "Mozilla Firefox Windows 11",
     .impersonate_target = kFirefoxTarget,
     .default_user_agent = kFirefoxWinUa,
     .sec_ch_ua_platform = ""},
    {.id = "safari_macos",
     .label = "Safari macOS",
     .impersonate_target = kSafariTarget,
     .default_user_agent = kSafariMacUa,
     .sec_ch_ua_platform = ""},
  };
  return presets;
}

//----------------------------------------
//
//----------------------------------------
const BrowserPreset* find_browser_preset(std::string_view preset_id) {
  const auto& presets = browser_presets();
  const auto it = std::find_if(presets.begin(), presets.end(),
                               [&](const BrowserPreset& preset) { return preset.id == preset_id; });
  if(it == presets.end()) {
    return nullptr;
  }
  return &(*it);
}

//----------------------------------------
//
//----------------------------------------
ClientIdentity resolve_client_identity(ClientIdentity identity) {
  if(identity.preset_id.empty()) {
    identity.preset_id = "none";
  }
  const BrowserPreset* preset = find_browser_preset(identity.preset_id);
  if(preset == nullptr) {
    preset = find_browser_preset("none");
  }
  if(preset == nullptr) {
    return identity;
  }
  identity.impersonate = std::string{preset->impersonate_target};
  if(identity.user_agent.empty()) {
    identity.user_agent = std::string{preset->default_user_agent};
  }
  return identity;
}

//----------------------------------------
//
//----------------------------------------
std::string browser_preset_dropdown_labels() {
  std::string labels;
  for(const auto& preset : browser_presets()) {
    if(not labels.empty()) {
      labels.push_back(';');
    }
    labels.append(preset.label);
  }
  return labels;
}

//----------------------------------------
//
//----------------------------------------
int browser_preset_index(std::string_view preset_id) {
  const auto& presets = browser_presets();
  for(std::size_t i = 0; i < presets.size(); ++i) {
    if(presets[i].id == preset_id) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

//----------------------------------------
//
//----------------------------------------
std::string_view browser_preset_id_at(int index) {
  const auto& presets = browser_presets();
  if(index < 0 || static_cast<std::size_t>(index) >= presets.size()) {
    return presets.front().id;
  }
  return presets[static_cast<std::size_t>(index)].id;
}

} // namespace requestman
