//----------------------------------------
// File:      curl_support.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "curl_support.hpp"

#include "http_header.hpp"

#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace requestman {
namespace {

struct CurlGlobal {
  CurlGlobal() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
  }
  ~CurlGlobal() {
    curl_global_cleanup();
  }
};

} // namespace

//----------------------------------------
//
//----------------------------------------
void ensure_curl_global() {
  static CurlGlobal global;
}

//----------------------------------------
//
//----------------------------------------
bool curl_transfer_size(std::size_t size, std::size_t nmemb, std::size_t& total) noexcept {
  if(size != 0 && nmemb > std::numeric_limits<std::size_t>::max() / size) {
    return false;
  }
  total = size * nmemb;
  return true;
}

//----------------------------------------
//
//----------------------------------------
void lock_curl_to_http(CURL* easy) {
#if LIBCURL_VERSION_NUM >= 0x075500
  curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "http,https");
  curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
  const auto http_protocols = static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS);
  curl_easy_setopt(easy, CURLOPT_PROTOCOLS, http_protocols);
  curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, http_protocols);
#endif
}

//----------------------------------------
//
//----------------------------------------
void set_curl_timeout(CURL* easy, std::chrono::milliseconds timeout) {
  curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));
}

//----------------------------------------
//
//----------------------------------------
void set_curl_upload_body(CURL* easy, std::string_view body) {
  curl_easy_setopt(easy, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
}

//----------------------------------------
//
//----------------------------------------
std::expected<void, Error> append_curl_header(CurlSlist& list, std::string_view name,
                                              std::string_view value) {
  if(contains_crlf(name) || contains_crlf(value)) {
    return std::unexpected(
      make_error(ErrorKind::Validation, "header contains a line break", std::string{name}));
  }
  list.append(std::string{name} + ": " + std::string{value});
  return {};
}

//----------------------------------------
//
//----------------------------------------
CurlSlist::~CurlSlist() {
  reset();
}

//----------------------------------------
//
//----------------------------------------
CurlSlist::CurlSlist(CurlSlist&& other) noexcept : list_(std::exchange(other.list_, nullptr)) {}

//----------------------------------------
//
//----------------------------------------
CurlSlist& CurlSlist::operator=(CurlSlist&& other) noexcept {
  if(this != &other) {
    reset();
    list_ = std::exchange(other.list_, nullptr);
  }
  return *this;
}

//----------------------------------------
//
//----------------------------------------
void CurlSlist::append(const std::string& line) {
  curl_slist* updated = curl_slist_append(list_, line.c_str());
  if(updated == nullptr) {
    throw std::bad_alloc{};
  }
  list_ = updated;
}

//----------------------------------------
//
//----------------------------------------
curl_slist* CurlSlist::get() const noexcept {
  return list_;
}

//----------------------------------------
//
//----------------------------------------
void CurlSlist::reset() noexcept {
  if(list_ != nullptr) {
    curl_slist_free_all(list_);
    list_ = nullptr;
  }
}

//----------------------------------------
//
//----------------------------------------
CurlEasy::CurlEasy() : handle_(curl_easy_init()) {
  if(handle_ == nullptr) {
    throw std::runtime_error("curl_easy_init failed");
  }
}

//----------------------------------------
//
//----------------------------------------
CurlEasy::~CurlEasy() {
  reset();
}

//----------------------------------------
//
//----------------------------------------
CurlEasy::CurlEasy(CurlEasy&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

//----------------------------------------
//
//----------------------------------------
CurlEasy& CurlEasy::operator=(CurlEasy&& other) noexcept {
  if(this != &other) {
    reset();
    handle_ = std::exchange(other.handle_, nullptr);
  }
  return *this;
}

//----------------------------------------
//
//----------------------------------------
CURL* CurlEasy::get() const noexcept {
  return handle_;
}

//----------------------------------------
//
//----------------------------------------
void CurlEasy::reset() noexcept {
  if(handle_ != nullptr) {
    curl_easy_cleanup(handle_);
    handle_ = nullptr;
  }
}

} // namespace requestman
