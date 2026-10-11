//----------------------------------------
// File:      curl_support.hpp
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

#include <chrono>
#include <cstddef>
#include <curl/curl.h>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace requestman {

void ensure_curl_global();

[[nodiscard]] bool curl_transfer_size(std::size_t size, std::size_t nmemb,
                                      std::size_t& total) noexcept;

void lock_curl_to_http(CURL* easy);

void set_curl_timeout(CURL* easy, std::chrono::milliseconds timeout);

void set_curl_upload_body(CURL* easy, std::string_view body);

class CurlSlist {
public:
  CurlSlist() = default;
  ~CurlSlist();

  CurlSlist(const CurlSlist&) = delete;
  CurlSlist& operator=(const CurlSlist&) = delete;
  CurlSlist(CurlSlist&& other) noexcept;
  CurlSlist& operator=(CurlSlist&& other) noexcept;

  void append(const std::string& line);
  [[nodiscard]] curl_slist* get() const noexcept;
  void reset() noexcept;

private:
  curl_slist* list_{nullptr};
};

class CurlEasy {
public:
  CurlEasy();
  ~CurlEasy();

  CurlEasy(const CurlEasy&) = delete;
  CurlEasy& operator=(const CurlEasy&) = delete;
  CurlEasy(CurlEasy&& other) noexcept;
  CurlEasy& operator=(CurlEasy&& other) noexcept;

  [[nodiscard]] CURL* get() const noexcept;
  void reset() noexcept;

private:
  CURL* handle_{nullptr};
};

[[nodiscard]] std::expected<void, Error> append_curl_header(CurlSlist& list, std::string_view name,
                                                            std::string_view value);

} // namespace requestman
