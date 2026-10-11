//----------------------------------------
// File:      request_executor.hpp
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
#include "http_client.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "ids.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <variant>
#include <vector>

namespace requestman {

struct RequestStartedEvent {
  CorrelationId correlation;
};

struct RequestFinishedEvent {
  CorrelationId correlation;
  std::expected<HttpResponse, Error> result;
};

using AppEvent = std::variant<RequestStartedEvent, RequestFinishedEvent>;

class RequestExecutor {
public:
  explicit RequestExecutor(std::shared_ptr<IHttpClient> client);
  ~RequestExecutor();

  RequestExecutor(const RequestExecutor&) = delete;
  RequestExecutor& operator=(const RequestExecutor&) = delete;

  [[nodiscard]] CorrelationId submit(HttpRequest request);
  void cancel_active();
  [[nodiscard]] std::vector<AppEvent> poll_events();

private:
  void worker_main();

  struct Job {
    CorrelationId correlation;
    HttpRequest request;
  };

  std::shared_ptr<IHttpClient> client_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::queue<Job> jobs_;
  bool stopping_{false};

  std::mutex events_mutex_;
  std::vector<AppEvent> events_;

  std::atomic_bool cancel_flag_{false};
  std::thread worker_;
  std::atomic<std::uint64_t> next_id_{1};
};

} // namespace requestman
