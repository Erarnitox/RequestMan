//----------------------------------------
// File:      paths.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-09-05
// Revision:  n/a
//----------------------------------------

#include "paths.hpp"

#include <cstdlib>
#include <system_error>

namespace requestman {

//----------------------------------------
//
//----------------------------------------
std::filesystem::path application_data_dir() {
  std::filesystem::path dir;
#if defined(_WIN32)
  if(const char* appdata = std::getenv("LOCALAPPDATA")) {
    dir = std::filesystem::path{appdata} / "RequestMan";
  } else if(const char* profile = std::getenv("USERPROFILE")) {
    dir = std::filesystem::path{profile} / "AppData" / "Local" / "RequestMan";
  } else {
    dir = std::filesystem::temp_directory_path() / "RequestMan";
  }
#elif defined(__APPLE__)
  if(const char* home = std::getenv("HOME")) {
    dir = std::filesystem::path{home} / "Library" / "Application Support" / "RequestMan";
  } else {
    dir = std::filesystem::temp_directory_path() / "RequestMan";
  }
#else
  if(const char* xdg = std::getenv("XDG_DATA_HOME")) {
    dir = std::filesystem::path{xdg} / "requestman";
  } else if(const char* home = std::getenv("HOME")) {
    dir = std::filesystem::path{home} / ".local" / "share" / "requestman";
  } else {
    dir = std::filesystem::temp_directory_path() / "requestman";
  }
#endif
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir;
}

//----------------------------------------
//
//----------------------------------------
std::filesystem::path default_database_path() {
  return application_data_dir() / "requestman.db";
}

//----------------------------------------
//
//----------------------------------------
std::filesystem::path test_database_path(std::string_view name) {
  auto dir = std::filesystem::temp_directory_path() / "requestman-tests";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir / (std::string{name} + ".db");
}

} // namespace requestman
