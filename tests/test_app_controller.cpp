//----------------------------------------
// File:      test_app_controller.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "app_controller.hpp"
#include "http_client.hpp"
#include "paths.hpp"
#include "sqlite_db.hpp"
#include "sqlite_repositories.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("app controller executes request asynchronously", "[application]") {
  const auto path = test_database_path("app-controller");
  std::error_code ec;
  std::filesystem::remove(path, ec);

  auto db = SqliteDb::open(path);
  REQUIRE(db);

  AppController controller(std::shared_ptr<IHttpClient>(make_curl_http_client()),
                           std::make_unique<SqliteHistoryRepository>(*db),
                           std::make_unique<SqliteSavedRequestRepository>(*db));

  HttpRequest request;
  request.method = HttpMethod::Get;
  request.url = "https://httpbin.org/get";
  request.settings.timeout = std::chrono::seconds{20};

  auto cid = controller.send_request(request);
  REQUIRE(cid);
  REQUIRE(controller.request_in_flight());

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
  while(controller.request_in_flight() && std::chrono::steady_clock::now() < deadline) {
    controller.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
  }
  controller.pump();

  REQUIRE_FALSE(controller.request_in_flight());
  REQUIRE(controller.last_result());
  REQUIRE(controller.last_result()->response);
  REQUIRE(controller.last_result()->response->status_code == 200);
  REQUIRE(controller.last_result()->resolved_request.url.find("httpbin.org") != std::string::npos);

  auto history = controller.history().list_entries(5);
  REQUIRE(history);
  REQUIRE_FALSE(history->empty());
}
