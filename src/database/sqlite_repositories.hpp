//----------------------------------------
// File:      sqlite_repositories.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "repositories.hpp"
#include "sqlite_db.hpp"

namespace requestman {

class SqliteHistoryRepository final : public IHistoryRepository {
public:
  explicit SqliteHistoryRepository(SqliteDb& db);

  [[nodiscard]] std::expected<HistoryEntry, Error> add_entry(HistoryEntry entry) override;
  [[nodiscard]] std::expected<std::vector<HistoryEntry>, Error>
  list_entries(std::size_t limit) override;
  [[nodiscard]] std::expected<HistoryEntry, Error> get_entry(HistoryEntryId id) override;
  [[nodiscard]] std::expected<void, Error> remove(HistoryEntryId id) override;
  [[nodiscard]] std::expected<void, Error> cleanup(std::size_t retention_limit) override;

private:
  SqliteDb* db_;
};

class SqliteSavedRequestRepository final : public ISavedRequestRepository {
public:
  explicit SqliteSavedRequestRepository(SqliteDb& db);

  [[nodiscard]] std::expected<std::vector<SavedRequest>, Error> list() override;
  [[nodiscard]] std::expected<SavedRequest, Error> get(SavedRequestId id) override;
  [[nodiscard]] std::expected<SavedRequest, Error> create(std::string name,
                                                          HttpRequest request) override;
  [[nodiscard]] std::expected<SavedRequest, Error> update(SavedRequestId id, std::string name,
                                                          HttpRequest request) override;
  [[nodiscard]] std::expected<void, Error> remove(SavedRequestId id) override;

private:
  SqliteDb* db_;
};

} // namespace requestman
