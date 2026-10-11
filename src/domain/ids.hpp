//----------------------------------------
// File:      ids.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace requestman {

struct HistoryEntryId {
  std::int64_t value{-1};

  [[nodiscard]] constexpr bool valid() const noexcept {
    return value > 0;
  }
  [[nodiscard]] friend constexpr bool operator==(HistoryEntryId, HistoryEntryId) = default;
  [[nodiscard]] friend constexpr auto operator<=>(HistoryEntryId, HistoryEntryId) = default;
};

struct SavedRequestId {
  std::int64_t value{-1};

  [[nodiscard]] constexpr bool valid() const noexcept {
    return value > 0;
  }
  [[nodiscard]] friend constexpr bool operator==(SavedRequestId, SavedRequestId) = default;
  [[nodiscard]] friend constexpr auto operator<=>(SavedRequestId, SavedRequestId) = default;
};

struct CorrelationId {
  std::string value;

  [[nodiscard]] bool empty() const noexcept {
    return value.empty();
  }
  [[nodiscard]] friend bool operator==(const CorrelationId&, const CorrelationId&) = default;
};

} // namespace requestman

template <>
struct std::hash<requestman::HistoryEntryId> {
  [[nodiscard]] std::size_t operator()(requestman::HistoryEntryId id) const noexcept {
    return std::hash<std::int64_t>{}(id.value);
  }
};

template <>
struct std::hash<requestman::SavedRequestId> {
  [[nodiscard]] std::size_t operator()(requestman::SavedRequestId id) const noexcept {
    return std::hash<std::int64_t>{}(id.value);
  }
};
