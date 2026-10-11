//----------------------------------------
// File:      http_client.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "error.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <atomic>
#include <expected>
#include <memory>

namespace requestman {

class IHttpClient {
public:
  virtual ~IHttpClient() = default;

  [[nodiscard]] virtual std::expected<HttpResponse, Error>
  send(const HttpRequest& request, std::atomic_bool* cancel_flag = nullptr) = 0;
};

[[nodiscard]] std::unique_ptr<IHttpClient> make_curl_http_client();

} // namespace requestman
