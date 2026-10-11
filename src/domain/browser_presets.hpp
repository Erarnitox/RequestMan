//----------------------------------------
// File:      browser_presets.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace requestman {

struct ClientIdentity {
  std::string preset_id{"none"};
  std::string impersonate; // curl-impersonate target, e.g. chrome146
  std::string user_agent;  // empty = library default for target (or RequestMan for none)
  bool apply_browser_headers{true};
};

struct BrowserPreset {
  std::string_view id;
  std::string_view label;
  std::string_view impersonate_target; // empty for none
  std::string_view default_user_agent;
  std::string_view sec_ch_ua_platform; // empty when N/A (e.g. Firefox)
};

[[nodiscard]] const std::vector<BrowserPreset>& browser_presets();

[[nodiscard]] const BrowserPreset* find_browser_preset(std::string_view preset_id);

// Fills impersonate / default UA from the catalog when preset_id is known.
// Leaves a custom user_agent untouched when already set.
[[nodiscard]] ClientIdentity resolve_client_identity(ClientIdentity identity);

[[nodiscard]] std::string browser_preset_dropdown_labels();

[[nodiscard]] int browser_preset_index(std::string_view preset_id);

[[nodiscard]] std::string_view browser_preset_id_at(int index);

} // namespace requestman
