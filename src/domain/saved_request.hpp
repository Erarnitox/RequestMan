//----------------------------------------
// File:      saved_request.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "http_request.hpp"
#include "ids.hpp"

#include <string>

namespace requestman {

struct SavedRequest {
  SavedRequestId id{};
  std::string name;
  HttpRequest request{};
  int sort_order{0};
};

} // namespace requestman
