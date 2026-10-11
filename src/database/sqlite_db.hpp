//----------------------------------------
// File:      sqlite_db.hpp
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

#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace requestman {

class SqliteDb {
public:
  SqliteDb() = default;
  ~SqliteDb();

  SqliteDb(const SqliteDb&) = delete;
  SqliteDb& operator=(const SqliteDb&) = delete;
  SqliteDb(SqliteDb&&) noexcept;
  SqliteDb& operator=(SqliteDb&&) noexcept;

  [[nodiscard]] static std::expected<SqliteDb, Error> open(const std::filesystem::path& path);

  [[nodiscard]] sqlite3* handle() const noexcept {
    return db_;
  }
  [[nodiscard]] std::expected<void, Error> exec(std::string_view sql) const;
  [[nodiscard]] std::expected<void, Error> begin() const;
  [[nodiscard]] std::expected<void, Error> commit() const;
  [[nodiscard]] std::expected<void, Error> rollback() const;

private:
  explicit SqliteDb(sqlite3* db) : db_(db) {}
  sqlite3* db_{nullptr};
};

class SqliteStatement {
public:
  SqliteStatement() = default;
  ~SqliteStatement();

  SqliteStatement(const SqliteStatement&) = delete;
  SqliteStatement& operator=(const SqliteStatement&) = delete;
  SqliteStatement(SqliteStatement&&) noexcept;
  SqliteStatement& operator=(SqliteStatement&&) noexcept;

  [[nodiscard]] static std::expected<SqliteStatement, Error> prepare(sqlite3* db,
                                                                     std::string_view sql);

  [[nodiscard]] sqlite3_stmt* handle() const noexcept {
    return stmt_;
  }

  [[nodiscard]] std::expected<void, Error> bind_int64(int index, std::int64_t value);
  [[nodiscard]] std::expected<void, Error> bind_int(int index, int value);
  [[nodiscard]] std::expected<void, Error> bind_text(int index, std::string_view value);
  [[nodiscard]] std::expected<void, Error> bind_blob(int index, std::span<const std::byte> value);
  [[nodiscard]] std::expected<void, Error> bind_null(int index);
  [[nodiscard]] std::expected<void, Error> reset();
  [[nodiscard]] std::expected<bool, Error> step(); // true => ROW

  [[nodiscard]] std::int64_t column_int64(int index) const;
  [[nodiscard]] int column_int(int index) const;
  [[nodiscard]] std::string column_text(int index) const;
  [[nodiscard]] std::vector<std::byte> column_blob(int index) const;

private:
  explicit SqliteStatement(sqlite3_stmt* stmt) : stmt_(stmt) {}
  sqlite3_stmt* stmt_{nullptr};
};

class SqliteTransaction {
public:
  explicit SqliteTransaction(SqliteDb& db);
  ~SqliteTransaction();

  SqliteTransaction(const SqliteTransaction&) = delete;
  SqliteTransaction& operator=(const SqliteTransaction&) = delete;

  [[nodiscard]] std::expected<void, Error> commit();

private:
  SqliteDb* db_{nullptr};
  bool open_{false};
};

[[nodiscard]] std::expected<void, Error> migrate_database(SqliteDb& db);

} // namespace requestman
