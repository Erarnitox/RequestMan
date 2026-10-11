//----------------------------------------
// File:      request_json.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "auth.hpp"
#include "error.hpp"
#include "http_request.hpp"

#include <expected>
#include <string>

namespace requestman {

[[nodiscard]] std::string serialize_auth(const AuthConfig& auth);
[[nodiscard]] std::expected<AuthConfig, Error> deserialize_auth(std::string_view json);

[[nodiscard]] std::string serialize_settings(const RequestSettings& settings);
[[nodiscard]] std::expected<RequestSettings, Error> deserialize_settings(std::string_view json);

[[nodiscard]] std::string serialize_request(const HttpRequest& request);
[[nodiscard]] std::expected<HttpRequest, Error> deserialize_request(std::string_view json);

} // namespace requestman
