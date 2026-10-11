//----------------------------------------
// File:      test_sqlite_repositories.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "paths.hpp"
#include "sqlite_db.hpp"
#include "sqlite_repositories.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>

using namespace requestman;

//----------------------------------------
//
//----------------------------------------
TEST_CASE("sqlite repositories persist history", "[database]") {
  const auto path = test_database_path("repos");
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::filesystem::remove(std::filesystem::path{path.string() + "-wal"}, ec);
  std::filesystem::remove(std::filesystem::path{path.string() + "-shm"}, ec);

  auto db = SqliteDb::open(path);
  REQUIRE(db);

  SqliteHistoryRepository history{*db};

  HttpRequest request;
  request.method = HttpMethod::Post;
  request.url = "https://example.com/users";
  request.headers.push_back({.name = "Content-Type", .value = "application/json", .enabled = true});
  request.query.push_back({.name = "verbose", .value = "1", .enabled = true});
  request.body = R"({"name":"ada"})";

  HistoryEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.method = HttpMethod::Get;
  entry.url = "https://example.com";
  entry.request = request;
  entry.status_code = 200;
  entry.duration = std::chrono::milliseconds{12};
  entry.response_size = 32;
  auto stored = history.add_entry(entry);
  REQUIRE(stored);
  auto listed = history.list_entries(10);
  REQUIRE(listed);
  REQUIRE_FALSE(listed->empty());
  REQUIRE(history.remove(stored->id));
  auto after_delete = history.list_entries(10);
  REQUIRE(after_delete);
  REQUIRE(after_delete->empty());
  REQUIRE(history.cleanup(1));
}

//----------------------------------------
//
//----------------------------------------
TEST_CASE("sqlite repositories persist saved requests", "[database]") {
  const auto path = test_database_path("saved-requests");
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::filesystem::remove(std::filesystem::path{path.string() + "-wal"}, ec);
  std::filesystem::remove(std::filesystem::path{path.string() + "-shm"}, ec);

  auto db = SqliteDb::open(path);
  REQUIRE(db);

  SqliteSavedRequestRepository saved{*db};

  HttpRequest request;
  request.method = HttpMethod::Post;
  request.url = "https://example.com/users";
  request.headers.push_back({.name = "Content-Type", .value = "application/json", .enabled = true});
  request.body = R"({"name":"ada"})";

  auto created = saved.create("POST /users", request);
  REQUIRE(created);
  REQUIRE(created->id.valid());
  REQUIRE(created->name == "POST /users");
  REQUIRE(created->request.body == request.body);
  REQUIRE(created->request.headers.size() == 1);

  auto listed = saved.list();
  REQUIRE(listed);
  REQUIRE(listed->size() == 1);

  request.body = R"({"name":"grace"})";
  auto updated = saved.update(created->id, created->name, request);
  REQUIRE(updated);
  REQUIRE(updated->request.body.find("grace") != std::string::npos);

  auto loaded = saved.get(created->id);
  REQUIRE(loaded);
  REQUIRE(loaded->request.body == updated->request.body);
  REQUIRE(saved.remove(created->id));
  auto after_delete = saved.list();
  REQUIRE(after_delete);
  REQUIRE(after_delete->empty());
}
