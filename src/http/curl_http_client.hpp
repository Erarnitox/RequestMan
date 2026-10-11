//----------------------------------------
// File:      curl_http_client.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_client.hpp"

namespace requestman {

class CurlHttpClient final : public IHttpClient {
public:
  [[nodiscard]] std::expected<HttpResponse, Error>
  send(const HttpRequest& request, std::atomic_bool* cancel_flag = nullptr) override;
};

} // namespace requestman
