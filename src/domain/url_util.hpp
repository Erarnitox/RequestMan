//----------------------------------------
// File:      url_util.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_header.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace requestman {

// Append enabled query parameters to a URL. Does not validate the base URL scheme.
[[nodiscard]] std::string append_query_parameters(std::string_view base_url,
                                                  const std::vector<HttpQueryParameter>& params);

[[nodiscard]] std::string percent_encode(std::string_view value);

// True for http, https, socks5, and socks5h proxy URLs.
[[nodiscard]] bool proxy_scheme_allowed(std::string_view url) noexcept;

} // namespace requestman
