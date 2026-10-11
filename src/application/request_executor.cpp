//----------------------------------------
// File:      request_executor.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "request_executor.hpp"

#include "http_method.hpp"
#include "logging.hpp"

#include <string>
#include <utility>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
RequestExecutor::RequestExecutor(std::shared_ptr<IHttpClient> client)
    : client_(std::move(client)), worker_([this] { worker_main(); }) {}

//----------------------------------------
//
//----------------------------------------
RequestExecutor::~RequestExecutor() {
  {
    std::lock_guard lock{queue_mutex_};
    stopping_ = true;
  }
  cancel_flag_.store(true, std::memory_order_relaxed);
  queue_cv_.notify_all();
  if(worker_.joinable()) {
    worker_.join();
  }
}

//----------------------------------------
//
//----------------------------------------
CorrelationId RequestExecutor::submit(HttpRequest request) {
  CorrelationId cid;
  cid.value = "req-" + std::to_string(next_id_.fetch_add(1, std::memory_order_relaxed));
  {
    std::lock_guard lock{queue_mutex_};
    jobs_.push(Job{.correlation = cid, .request = std::move(request)});
  }
  {
    std::lock_guard lock{events_mutex_};
    events_.push_back(RequestStartedEvent{.correlation = cid});
  }
  queue_cv_.notify_one();
  return cid;
}

//----------------------------------------
//
//----------------------------------------
void RequestExecutor::cancel_active() {
  cancel_flag_.store(true, std::memory_order_relaxed);
}

//----------------------------------------
//
//----------------------------------------
std::vector<AppEvent> RequestExecutor::poll_events() {
  std::lock_guard lock{events_mutex_};
  std::vector<AppEvent> out;
  out.swap(events_);
  return out;
}

//----------------------------------------
//
//----------------------------------------
void RequestExecutor::worker_main() {
  while(true) {
    Job job;
    {
      std::unique_lock lock{queue_mutex_};
      queue_cv_.wait(lock, [&] { return stopping_ || not jobs_.empty(); });
      if(stopping_ && jobs_.empty()) {
        return;
      }
      job = std::move(jobs_.front());
      jobs_.pop();
    }

    cancel_flag_.store(false, std::memory_order_relaxed);
    log_info(std::string{"executing "} + std::string{to_string(job.request.method)} + " "
               + redact_for_log(job.request.url),
             job.correlation.value);
    auto result = client_->send(job.request, &cancel_flag_);
    if(not result) {
      log_error(redact_for_log(result.error().full_message()), job.correlation.value);
    } else {
      log_info("request completed status=" + std::to_string(result->status_code),
               job.correlation.value);
    }

    std::lock_guard lock{events_mutex_};
    events_.push_back(
      RequestFinishedEvent{.correlation = std::move(job.correlation), .result = std::move(result)});
  }
}

} // namespace requestman
