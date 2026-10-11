//----------------------------------------
// File:      app_controller.hpp
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
#include "history.hpp"
#include "http_client.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "repositories.hpp"
#include "request_executor.hpp"

#include <expected>
#include <memory>
#include <optional>

namespace requestman {

struct ExecutionResult {
  CorrelationId correlation;
  HttpRequest resolved_request;
  std::expected<HttpResponse, Error> response;
};

class AppController {
public:
  AppController(std::shared_ptr<IHttpClient> http, std::unique_ptr<IHistoryRepository> history,
                std::unique_ptr<ISavedRequestRepository> saved_requests);

  [[nodiscard]] IHistoryRepository& history() {
    return *history_;
  }
  [[nodiscard]] ISavedRequestRepository& saved_requests() {
    return *saved_requests_;
  }
  [[nodiscard]] RequestExecutor& executor() {
    return *executor_;
  }

  [[nodiscard]] std::expected<CorrelationId, Error> send_request(HttpRequest request);
  void pump();

  [[nodiscard]] const std::optional<ExecutionResult>& last_result() const {
    return last_result_;
  }
  [[nodiscard]] bool request_in_flight() const {
    return in_flight_;
  }

private:
  std::unique_ptr<IHistoryRepository> history_;
  std::unique_ptr<ISavedRequestRepository> saved_requests_;
  std::unique_ptr<RequestExecutor> executor_;
  std::optional<HttpRequest> pending_resolved_;
  std::optional<CorrelationId> active_correlation_;
  std::optional<ExecutionResult> last_result_;
  bool in_flight_{false};
};

} // namespace requestman
