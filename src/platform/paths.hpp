//----------------------------------------
// File:      paths.hpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#pragma once

#include <filesystem>
#include <string>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::filesystem::path application_data_dir();
[[nodiscard]] std::filesystem::path default_database_path();
[[nodiscard]] std::filesystem::path test_database_path(std::string_view name);

} // namespace requestman
