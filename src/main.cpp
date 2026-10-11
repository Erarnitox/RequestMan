//----------------------------------------
// File:      main.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#include "app_controller.hpp"
#include "http_client.hpp"
#include "logging.hpp"
#include "main_window.hpp"
#include "paths.hpp"
#include "sqlite_db.hpp"
#include "sqlite_repositories.hpp"

#include <iostream>
#include <memory>

//----------------------------------------
//
//----------------------------------------
int main() {
  using namespace requestman;

  set_log_level(LogLevel::Info);

  auto db = SqliteDb::open(default_database_path());
  if(not db) {
    std::cerr << "failed to open database: " << db.error().full_message() << '\n';

    return 1;
  }

  AppController controller(std::shared_ptr<IHttpClient>(make_curl_http_client()),
                           std::make_unique<SqliteHistoryRepository>(*db),
                           std::make_unique<SqliteSavedRequestRepository>(*db));

  MainWindow window{controller};
  window.run();
  return 0;
}
