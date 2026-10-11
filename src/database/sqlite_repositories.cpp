//----------------------------------------
// File:      sqlite_repositories.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "sqlite_repositories.hpp"

#include "request_json.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sqlite3.h>
#include <sstream>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string format_time(std::chrono::system_clock::time_point tp) {
  const std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::chrono::system_clock::time_point parse_time(std::string_view text) {
  std::tm tm{};
  std::istringstream iss{std::string{text}};
  iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  if(iss.fail()) {
    return std::chrono::system_clock::now();
  }
#if defined(_WIN32)
  const time_t t = _mkgmtime(&tm);
#else
  const time_t t = timegm(&tm);
#endif
  return std::chrono::system_clock::from_time_t(t);
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::int64_t last_id(sqlite3* db) {
  return sqlite3_last_insert_rowid(db);
}

} // namespace

//----------------------------------------
//
//----------------------------------------
SqliteHistoryRepository::SqliteHistoryRepository(SqliteDb& db) : db_(&db) {}

//----------------------------------------
//
//----------------------------------------
std::expected<HistoryEntry, Error> SqliteHistoryRepository::add_entry(HistoryEntry entry) {
  auto stmt = SqliteStatement::prepare(db_->handle(),
                                       "INSERT INTO history(timestamp, method, url, request_json, "
                                       "status_code, duration_ms, response_size, error_message) "
                                       "VALUES(?1,?2,?3,?4,?5,?6,?7,?8)");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_text(1, format_time(entry.timestamp)); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_text(2, to_string(entry.method)); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_text(3, entry.url); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_text(4, serialize_request(entry.request)); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_int(5, entry.status_code); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_int64(6, entry.duration.count()); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_int64(7, static_cast<std::int64_t>(entry.response_size)); not r) {
    return std::unexpected(r.error());
  }
  if(entry.error_message) {
    if(auto r = stmt->bind_text(8, *entry.error_message); not r) {
      return std::unexpected(r.error());
    }
  } else if(auto r = stmt->bind_null(8); not r) {
    return std::unexpected(r.error());
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }
  entry.id = HistoryEntryId{last_id(db_->handle())};
  return entry;
}

//----------------------------------------
//
//----------------------------------------
std::expected<std::vector<HistoryEntry>, Error>
SqliteHistoryRepository::list_entries(std::size_t limit) {
  auto stmt = SqliteStatement::prepare(db_->handle(),
                                       "SELECT id, timestamp, method, url, request_json, "
                                       "status_code, duration_ms, response_size, error_message "
                                       "FROM history ORDER BY id DESC LIMIT ?1");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, static_cast<std::int64_t>(limit)); not r) {
    return std::unexpected(r.error());
  }
  std::vector<HistoryEntry> out;
  while(true) {
    auto row = stmt->step();
    if(not row) {
      return std::unexpected(row.error());
    }
    if(not *row) {
      break;
    }
    HistoryEntry entry;
    entry.id = HistoryEntryId{stmt->column_int64(0)};
    entry.timestamp = parse_time(stmt->column_text(1));
    HttpMethod method{};
    (void)try_parse_http_method(stmt->column_text(2), method);
    entry.method = method;
    entry.url = stmt->column_text(3);
    auto request = deserialize_request(stmt->column_text(4));
    if(request) {
      entry.request = std::move(*request);
    }
    entry.status_code = stmt->column_int(5);
    entry.duration = std::chrono::milliseconds{stmt->column_int64(6)};
    entry.response_size = static_cast<std::size_t>(stmt->column_int64(7));
    const auto err = stmt->column_text(8);
    if(not err.empty()) {
      entry.error_message = err;
    }
    out.push_back(std::move(entry));
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::expected<HistoryEntry, Error> SqliteHistoryRepository::get_entry(HistoryEntryId id) {
  auto stmt = SqliteStatement::prepare(db_->handle(),
                                       "SELECT id, timestamp, method, url, request_json, "
                                       "status_code, duration_ms, response_size, error_message "
                                       "FROM history WHERE id=?1");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, id.value); not r) {
    return std::unexpected(r.error());
  }
  auto row = stmt->step();
  if(not row) {
    return std::unexpected(row.error());
  }
  if(not *row) {
    return std::unexpected(make_error(ErrorKind::Storage, "history entry not found"));
  }
  HistoryEntry entry;
  entry.id = HistoryEntryId{stmt->column_int64(0)};
  entry.timestamp = parse_time(stmt->column_text(1));
  HttpMethod method{};
  (void)try_parse_http_method(stmt->column_text(2), method);
  entry.method = method;
  entry.url = stmt->column_text(3);
  auto request = deserialize_request(stmt->column_text(4));
  if(not request) {
    return std::unexpected(request.error());
  }
  entry.request = std::move(*request);
  entry.status_code = stmt->column_int(5);
  entry.duration = std::chrono::milliseconds{stmt->column_int64(6)};
  entry.response_size = static_cast<std::size_t>(stmt->column_int64(7));
  const auto err = stmt->column_text(8);
  if(not err.empty()) {
    entry.error_message = err;
  }
  return entry;
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteHistoryRepository::remove(HistoryEntryId id) {
  auto stmt = SqliteStatement::prepare(db_->handle(), "DELETE FROM history WHERE id=?1");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, id.value); not r) {
    return r;
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteHistoryRepository::cleanup(std::size_t retention_limit) {
  auto stmt = SqliteStatement::prepare(
    db_->handle(),
    "DELETE FROM history WHERE id NOT IN (SELECT id FROM history ORDER BY id DESC LIMIT ?1)");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, static_cast<std::int64_t>(retention_limit)); not r) {
    return r;
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
SqliteSavedRequestRepository::SqliteSavedRequestRepository(SqliteDb& db) : db_(&db) {}

//----------------------------------------
//
//----------------------------------------
std::expected<std::vector<SavedRequest>, Error> SqliteSavedRequestRepository::list() {
  auto stmt =
    SqliteStatement::prepare(db_->handle(), "SELECT id, name, request_json, sort_order FROM "
                                            "saved_requests ORDER BY sort_order ASC, id ASC");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  std::vector<SavedRequest> out;
  while(true) {
    auto row = stmt->step();
    if(not row) {
      return std::unexpected(row.error());
    }
    if(not *row) {
      break;
    }
    SavedRequest saved;
    saved.id = SavedRequestId{stmt->column_int64(0)};
    saved.name = stmt->column_text(1);
    auto request = deserialize_request(stmt->column_text(2));
    if(not request) {
      return std::unexpected(request.error());
    }
    saved.request = std::move(*request);
    saved.sort_order = stmt->column_int(3);
    out.push_back(std::move(saved));
  }
  return out;
}

//----------------------------------------
//
//----------------------------------------
std::expected<SavedRequest, Error> SqliteSavedRequestRepository::get(SavedRequestId id) {
  auto stmt = SqliteStatement::prepare(
    db_->handle(), "SELECT id, name, request_json, sort_order FROM saved_requests WHERE id=?1");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, id.value); not r) {
    return std::unexpected(r.error());
  }
  auto row = stmt->step();
  if(not row) {
    return std::unexpected(row.error());
  }
  if(not *row) {
    return std::unexpected(make_error(ErrorKind::Storage, "saved request not found"));
  }
  SavedRequest saved;
  saved.id = SavedRequestId{stmt->column_int64(0)};
  saved.name = stmt->column_text(1);
  auto request = deserialize_request(stmt->column_text(2));
  if(not request) {
    return std::unexpected(request.error());
  }
  saved.request = std::move(*request);
  saved.sort_order = stmt->column_int(3);
  return saved;
}

//----------------------------------------
//
//----------------------------------------
std::expected<SavedRequest, Error> SqliteSavedRequestRepository::create(std::string name,
                                                                        HttpRequest request) {
  if(name.empty()) {
    return std::unexpected(make_error(ErrorKind::Validation, "saved request name is required"));
  }

  auto order_stmt = SqliteStatement::prepare(
    db_->handle(), "SELECT COALESCE(MAX(sort_order), -1) + 1 FROM saved_requests");
  if(not order_stmt) {
    return std::unexpected(order_stmt.error());
  }
  auto order_row = order_stmt->step();
  if(not order_row) {
    return std::unexpected(order_row.error());
  }
  const int sort_order = order_row.value() ? order_stmt->column_int(0) : 0;

  auto stmt = SqliteStatement::prepare(
    db_->handle(), "INSERT INTO saved_requests(name, request_json, sort_order) VALUES(?1,?2,?3)");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_text(1, name); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_text(2, serialize_request(request)); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_int(3, sort_order); not r) {
    return std::unexpected(r.error());
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }

  return SavedRequest{.id = SavedRequestId{last_id(db_->handle())},
                      .name = std::move(name),
                      .request = std::move(request),
                      .sort_order = sort_order};
}

//----------------------------------------
//
//----------------------------------------
std::expected<SavedRequest, Error>
SqliteSavedRequestRepository::update(SavedRequestId id, std::string name, HttpRequest request) {
  if(not id.valid()) {
    return std::unexpected(make_error(ErrorKind::Validation, "saved request id is invalid"));
  }
  if(name.empty()) {
    return std::unexpected(make_error(ErrorKind::Validation, "saved request name is required"));
  }

  auto existing = get(id);
  if(not existing) {
    return std::unexpected(existing.error());
  }

  auto stmt = SqliteStatement::prepare(
    db_->handle(), "UPDATE saved_requests SET name=?1, request_json=?2 WHERE id=?3");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_text(1, name); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_text(2, serialize_request(request)); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = stmt->bind_int64(3, id.value); not r) {
    return std::unexpected(r.error());
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }

  existing->name = std::move(name);
  existing->request = std::move(request);
  return *existing;
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteSavedRequestRepository::remove(SavedRequestId id) {
  auto stmt = SqliteStatement::prepare(db_->handle(), "DELETE FROM saved_requests WHERE id=?1");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  if(auto r = stmt->bind_int64(1, id.value); not r) {
    return r;
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }
  return {};
}

} // namespace requestman
