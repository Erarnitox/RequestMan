//----------------------------------------
// File:      auth_apply.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#pragma once

#include "auth.hpp"
#include "http_request.hpp"

namespace requestman {

// Enrich a request with authentication material. Does not perform I/O.
[[nodiscard]] HttpRequest apply_authentication(HttpRequest request);

} // namespace requestman
