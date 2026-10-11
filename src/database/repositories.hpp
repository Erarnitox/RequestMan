//----------------------------------------
// File:      repositories.hpp
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
#include "history.hpp"
#include "saved_request.hpp"

#include <expected>
#include <string>
#include <vector>

namespace requestman {

class IHistoryRepository {
public:
  virtual ~IHistoryRepository() = default;

  [[nodiscard]] virtual std::expected<HistoryEntry, Error> add_entry(HistoryEntry entry) = 0;
  [[nodiscard]] virtual std::expected<std::vector<HistoryEntry>, Error>
  list_entries(std::size_t limit) = 0;
  [[nodiscard]] virtual std::expected<HistoryEntry, Error> get_entry(HistoryEntryId id) = 0;
  [[nodiscard]] virtual std::expected<void, Error> remove(HistoryEntryId id) = 0;
  [[nodiscard]] virtual std::expected<void, Error> cleanup(std::size_t retention_limit) = 0;
};

class ISavedRequestRepository {
public:
  virtual ~ISavedRequestRepository() = default;

  [[nodiscard]] virtual std::expected<std::vector<SavedRequest>, Error> list() = 0;
  [[nodiscard]] virtual std::expected<SavedRequest, Error> get(SavedRequestId id) = 0;
  [[nodiscard]] virtual std::expected<SavedRequest, Error> create(std::string name,
                                                                  HttpRequest request) = 0;
  [[nodiscard]] virtual std::expected<SavedRequest, Error>
  update(SavedRequestId id, std::string name, HttpRequest request) = 0;
  [[nodiscard]] virtual std::expected<void, Error> remove(SavedRequestId id) = 0;
};

} // namespace requestman
