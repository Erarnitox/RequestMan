//----------------------------------------
// File:      app_controller.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "app_controller.hpp"

#include "logging.hpp"
#include "oauth2_token.hpp"

#include <chrono>
#include <utility>
#include <variant>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
AppController::AppController(std::shared_ptr<IHttpClient> http,
                             std::unique_ptr<IHistoryRepository> history,
                             std::unique_ptr<ISavedRequestRepository> saved_requests)
    : history_(std::move(history)), saved_requests_(std::move(saved_requests)),
      executor_(std::make_unique<RequestExecutor>(std::move(http))) {}

//----------------------------------------
//
//----------------------------------------
std::expected<CorrelationId, Error> AppController::send_request(HttpRequest request) {
  if(in_flight_) {
    return std::unexpected(make_error(ErrorKind::Validation, "a request is already in flight"));
  }

  if(auto ensured = ensure_oauth2_access_token(request); not ensured) {
    return std::unexpected(ensured.error());
  }

  pending_resolved_ = request;
  const auto cid = executor_->submit(std::move(request));
  active_correlation_ = cid;
  in_flight_ = true;
  return cid;
}

//----------------------------------------
//
//----------------------------------------
void AppController::pump() {
  for(auto& event : executor_->poll_events()) {
    std::visit(
      [&](auto&& ev) {
        using T = std::decay_t<decltype(ev)>;
        if constexpr(std::is_same_v<T, RequestFinishedEvent>) {
          if(active_correlation_ && ev.correlation.value == active_correlation_->value) {
            in_flight_ = false;
          }

          ExecutionResult result;
          result.correlation = ev.correlation;
          if(pending_resolved_) {
            result.resolved_request = *pending_resolved_;
          }
          result.response = std::move(ev.result);

          HistoryEntry entry;
          entry.timestamp = std::chrono::system_clock::now();
          entry.method = result.resolved_request.method;
          entry.url = result.resolved_request.url;
          entry.request = result.resolved_request;
          if(result.response) {
            entry.status_code = result.response->status_code;
            entry.duration = result.response->elapsed;
            entry.response_size = result.response->body.size();
          } else {
            entry.error_message = redact_for_log(result.response.error().full_message());
          }
          auto stored = history_->add_entry(std::move(entry));
          if(not stored) {
            log_error(stored.error().full_message());
          } else {
            (void)history_->cleanup(kDefaultHistoryRetentionLimit);
          }

          last_result_ = std::move(result);
          pending_resolved_.reset();
          active_correlation_.reset();
        }
      },
      event);
  }
}

} // namespace requestman
