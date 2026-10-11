//----------------------------------------
// File:      body_format.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_response.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace requestman {

[[nodiscard]] BodyKind detect_body_kind(std::span<const std::byte> body,
                                        std::string_view content_type = {});

[[nodiscard]] std::string body_as_lossy_text(std::span<const std::byte> body);

[[nodiscard]] std::string format_json_pretty(std::string_view json_text);

[[nodiscard]] std::string format_xml_pretty(std::string_view xml_text);

[[nodiscard]] std::string format_response_for_display(const HttpResponse& response, bool pretty,
                                                      std::size_t max_chars);

} // namespace requestman
