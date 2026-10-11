//----------------------------------------
// File:      json_util.hpp
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

namespace requestman {

[[nodiscard]] std::string json_escape(std::string_view value);

[[nodiscard]] inline std::string json_quote(std::string_view value) {
  return "\"" + json_escape(value) + "\"";
}

[[nodiscard]] bool extract_json_string(std::string_view json, std::string_view key,
                                       std::string& out);

[[nodiscard]] bool extract_json_int(std::string_view json, std::string_view key, long long& out);

[[nodiscard]] bool extract_json_bool(std::string_view json, std::string_view key, bool& out);

} // namespace requestman
