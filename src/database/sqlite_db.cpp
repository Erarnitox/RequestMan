//----------------------------------------
// File:      sqlite_db.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "sqlite_db.hpp"

#include <cstring>
#include <limits>
#include <sqlite3.h>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] Error sqlite_error(sqlite3* db, std::string message) {
  const char* err = db != nullptr ? sqlite3_errmsg(db) : "unknown sqlite error";
  return make_error(ErrorKind::Storage, std::move(message), err != nullptr ? err : "");
}

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::expected<int, Error> sqlite_bind_size(std::size_t size) {
  if(size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    return std::unexpected(make_error(ErrorKind::Storage, "value is too large to bind"));
  }
  return static_cast<int>(size);
}

} // namespace

//----------------------------------------
//
//----------------------------------------
SqliteDb::~SqliteDb() {
  if(db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

//----------------------------------------
//
//----------------------------------------
SqliteDb::SqliteDb(SqliteDb&& other) noexcept : db_(std::exchange(other.db_, nullptr)) {}

//----------------------------------------
//
//----------------------------------------
SqliteDb& SqliteDb::operator=(SqliteDb&& other) noexcept {
  if(this != &other) {
    if(db_ != nullptr) {
      sqlite3_close(db_);
    }
    db_ = std::exchange(other.db_, nullptr);
  }
  return *this;
}

//----------------------------------------
//
//----------------------------------------
std::expected<SqliteDb, Error> SqliteDb::open(const std::filesystem::path& path) {
  sqlite3* raw = nullptr;
  const int rc =
    sqlite3_open_v2(path.string().c_str(), &raw,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
  if(rc != SQLITE_OK) {
    const auto err = sqlite_error(raw, "failed to open database");
    if(raw != nullptr) {
      sqlite3_close(raw);
    }
    return std::unexpected(err);
  }

  SqliteDb db{raw};
  if(auto r = db.exec("PRAGMA foreign_keys = ON"); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = db.exec("PRAGMA journal_mode = WAL"); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = db.exec("PRAGMA busy_timeout = 5000"); not r) {
    return std::unexpected(r.error());
  }
  if(auto r = migrate_database(db); not r) {
    return std::unexpected(r.error());
  }
  return db;
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteDb::exec(std::string_view sql) const {
  char* err_msg = nullptr;
  const int rc = sqlite3_exec(db_, std::string{sql}.c_str(), nullptr, nullptr, &err_msg);
  if(rc != SQLITE_OK) {
    std::string detail = err_msg != nullptr ? err_msg : sqlite3_errmsg(db_);
    sqlite3_free(err_msg);
    return std::unexpected(make_error(ErrorKind::Storage, "sqlite exec failed", detail));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteDb::begin() const {
  return exec("BEGIN IMMEDIATE");
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteDb::commit() const {
  return exec("COMMIT");
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteDb::rollback() const {
  return exec("ROLLBACK");
}

//----------------------------------------
//
//----------------------------------------
SqliteStatement::~SqliteStatement() {
  if(stmt_ != nullptr) {
    sqlite3_finalize(stmt_);
    stmt_ = nullptr;
  }
}

//----------------------------------------
//
//----------------------------------------
SqliteStatement::SqliteStatement(SqliteStatement&& other) noexcept
    : stmt_(std::exchange(other.stmt_, nullptr)) {}

//----------------------------------------
//
//----------------------------------------
SqliteStatement& SqliteStatement::operator=(SqliteStatement&& other) noexcept {
  if(this != &other) {
    if(stmt_ != nullptr) {
      sqlite3_finalize(stmt_);
    }
    stmt_ = std::exchange(other.stmt_, nullptr);
  }
  return *this;
}

//----------------------------------------
//
//----------------------------------------
std::expected<SqliteStatement, Error> SqliteStatement::prepare(sqlite3* db, std::string_view sql) {
  sqlite3_stmt* stmt = nullptr;
  const int rc = sqlite3_prepare_v2(db, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
  if(rc != SQLITE_OK) {
    return std::unexpected(sqlite_error(db, "failed to prepare statement"));
  }
  return SqliteStatement{stmt};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::bind_int64(int index, std::int64_t value) {
  if(sqlite3_bind_int64(stmt_, index, value) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "bind_int64 failed"));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::bind_int(int index, int value) {
  if(sqlite3_bind_int(stmt_, index, value) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "bind_int failed"));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::bind_text(int index, std::string_view value) {
  auto size = sqlite_bind_size(value.size());
  if(not size) {
    return std::unexpected(size.error());
  }
  if(sqlite3_bind_text(stmt_, index, value.data(), *size, SQLITE_TRANSIENT) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "bind_text failed"));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::bind_blob(int index, std::span<const std::byte> value) {
  auto size = sqlite_bind_size(value.size());
  if(not size) {
    return std::unexpected(size.error());
  }
  if(sqlite3_bind_blob(stmt_, index, value.data(), *size, SQLITE_TRANSIENT) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "bind_blob failed"));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::bind_null(int index) {
  if(sqlite3_bind_null(stmt_, index) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "bind_null failed"));
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteStatement::reset() {
  if(sqlite3_reset(stmt_) != SQLITE_OK) {
    return std::unexpected(make_error(ErrorKind::Storage, "statement reset failed"));
  }
  sqlite3_clear_bindings(stmt_);
  return {};
}

//----------------------------------------
//
//----------------------------------------
std::expected<bool, Error> SqliteStatement::step() {
  const int rc = sqlite3_step(stmt_);
  if(rc == SQLITE_ROW) {
    return true;
  }
  if(rc == SQLITE_DONE) {
    return false;
  }
  return std::unexpected(
    make_error(ErrorKind::Storage, "statement step failed", sqlite3_errstr(rc)));
}

//----------------------------------------
//
//----------------------------------------
std::int64_t SqliteStatement::column_int64(int index) const {
  return sqlite3_column_int64(stmt_, index);
}

//----------------------------------------
//
//----------------------------------------
int SqliteStatement::column_int(int index) const {
  return sqlite3_column_int(stmt_, index);
}

//----------------------------------------
//
//----------------------------------------
std::string SqliteStatement::column_text(int index) const {
  const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
  if(text == nullptr) {
    return {};
  }
  return std::string{text};
}

//----------------------------------------
//
//----------------------------------------
std::vector<std::byte> SqliteStatement::column_blob(int index) const {
  const auto* data = static_cast<const std::byte*>(sqlite3_column_blob(stmt_, index));
  const int size = sqlite3_column_bytes(stmt_, index);
  if(data == nullptr || size <= 0) {
    return {};
  }
  return std::vector<std::byte>{data, data + size};
}

//----------------------------------------
//
//----------------------------------------
SqliteTransaction::SqliteTransaction(SqliteDb& db) : db_(&db) {
  if(db_->begin()) {
    open_ = true;
  }
}

//----------------------------------------
//
//----------------------------------------
SqliteTransaction::~SqliteTransaction() {
  if(open_ && db_ != nullptr) {
    (void)db_->rollback();
  }
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> SqliteTransaction::commit() {
  if(not open_ || db_ == nullptr) {
    return std::unexpected(make_error(ErrorKind::Storage, "transaction is not open"));
  }
  auto result = db_->commit();
  open_ = false;
  return result;
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> migrate_database(SqliteDb& db) {
  if(auto r = db.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS schema_migrations (
      version INTEGER PRIMARY KEY,
      applied_at TEXT NOT NULL DEFAULT (datetime('now'))
    );
  )SQL");
     not r) {
    return r;
  }

  auto stmt = SqliteStatement::prepare(db.handle(),
                                       "SELECT COALESCE(MAX(version), 0) FROM schema_migrations");
  if(not stmt) {
    return std::unexpected(stmt.error());
  }
  auto stepped = stmt->step();
  if(not stepped) {
    return std::unexpected(stepped.error());
  }
  const int current = stepped.value() ? stmt->column_int(0) : 0;

  if(current < 1) {
    if(auto r = db.exec(R"SQL(
      CREATE TABLE history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT NOT NULL,
        method TEXT NOT NULL,
        url TEXT NOT NULL,
        request_json TEXT NOT NULL,
        status_code INTEGER NOT NULL DEFAULT 0,
        duration_ms INTEGER NOT NULL DEFAULT 0,
        response_size INTEGER NOT NULL DEFAULT 0,
        error_message TEXT
      );

      CREATE INDEX idx_history_timestamp ON history(timestamp DESC);
    )SQL");
       not r) {
      return r;
    }

    if(auto r = db.exec("INSERT INTO schema_migrations(version) VALUES (1)"); not r) {
      return r;
    }
  }

  if(current < 2) {
    if(auto r = db.exec(R"SQL(
      DROP TABLE IF EXISTS request_cookies;
      DROP TABLE IF EXISTS request_query_params;
      DROP TABLE IF EXISTS request_headers;
      DROP TABLE IF EXISTS saved_requests;
      DROP TABLE IF EXISTS collections;
    )SQL");
       not r) {
      return r;
    }
    if(auto r = db.exec("INSERT INTO schema_migrations(version) VALUES (2)"); not r) {
      return r;
    }
  }

  if(current < 3) {
    if(auto r = db.exec(R"SQL(
      DROP TABLE IF EXISTS environment_variables;
      DROP TABLE IF EXISTS environments;
    )SQL");
       not r) {
      return r;
    }
    if(auto r = db.exec("INSERT INTO schema_migrations(version) VALUES (3)"); not r) {
      return r;
    }
  }

  if(current < 4) {
    if(auto r = db.exec(R"SQL(
      CREATE TABLE saved_requests (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        request_json TEXT NOT NULL,
        sort_order INTEGER NOT NULL DEFAULT 0
      );
    )SQL");
       not r) {
      return r;
    }
    if(auto r =
         db.exec("CREATE INDEX idx_saved_requests_sort ON saved_requests(sort_order ASC, id ASC)");
       not r) {
      return r;
    }
    if(auto r = db.exec("INSERT INTO schema_migrations(version) VALUES (4)"); not r) {
      return r;
    }
  }

  return {};
}

} // namespace requestman
