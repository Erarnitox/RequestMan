//----------------------------------------
// File:      request_json.cpp
// Author:    erarnitox - David Partsch
// Email:     david@erarnitox.de
// Website:   https://erarnitox.de
// Copyright: (c) 2026 David Partsch
// License:   MIT
// Date:      2026-10-11
// Revision:  n/a
//----------------------------------------

#include "request_json.hpp"

#include "http_method.hpp"
#include "json_util.hpp"

#include <chrono>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace requestman {
namespace {

//----------------------------------------
//
//----------------------------------------
[[nodiscard]] std::string_view extract_object_array(std::string_view json, std::string_view key) {
  const std::string pattern = "\"" + std::string{key} + "\"";
  auto pos = json.find(pattern);
  if(pos == std::string_view::npos) {
    return {};
  }
  pos = json.find('[', pos);
  if(pos == std::string_view::npos) {
    return {};
  }
  int depth = 0;
  for(std::size_t i = pos; i < json.size(); ++i) {
    if(json[i] == '[') {
      ++depth;
    } else if(json[i] == ']') {
      --depth;
      if(depth == 0) {
        return json.substr(pos, i - pos + 1);
      }
    }
  }
  return {};
}

//----------------------------------------
//
//----------------------------------------
void parse_key_values(std::string_view array_json, std::vector<HttpHeader>& headers) {
  std::size_t pos = 0;
  while(true) {
    auto start = array_json.find('{', pos);
    if(start == std::string_view::npos) {
      break;
    }
    auto end = array_json.find('}', start);
    if(end == std::string_view::npos) {
      break;
    }
    auto obj = array_json.substr(start, end - start + 1);
    HttpHeader header;
    (void)extract_json_string(obj, "name", header.name);
    (void)extract_json_string(obj, "value", header.value);
    bool enabled = true;
    (void)extract_json_bool(obj, "enabled", enabled);
    header.enabled = enabled;
    headers.push_back(std::move(header));
    pos = end + 1;
  }
}

//----------------------------------------
//
//----------------------------------------
void parse_query(std::string_view array_json, std::vector<HttpQueryParameter>& params) {
  std::size_t pos = 0;
  while(true) {
    auto start = array_json.find('{', pos);
    if(start == std::string_view::npos) {
      break;
    }
    auto end = array_json.find('}', start);
    if(end == std::string_view::npos) {
      break;
    }
    auto obj = array_json.substr(start, end - start + 1);
    HttpQueryParameter param;
    (void)extract_json_string(obj, "name", param.name);
    (void)extract_json_string(obj, "value", param.value);
    bool enabled = true;
    (void)extract_json_bool(obj, "enabled", enabled);
    param.enabled = enabled;
    params.push_back(std::move(param));
    pos = end + 1;
  }
}

} // namespace

//----------------------------------------
//
//----------------------------------------
std::string serialize_auth(const AuthConfig& auth) {
  return std::visit(
    [](const auto& value) -> std::string {
      using T = std::decay_t<decltype(value)>;
      if constexpr(std::is_same_v<T, NoAuth>) {
        return R"({"type":"none"})";
      } else if constexpr(std::is_same_v<T, BearerAuth>) {
        return std::string{"{\"type\":\"bearer\",\"token\":"} + json_quote(value.token) + "}";
      } else if constexpr(std::is_same_v<T, BasicAuth>) {
        return std::string{"{\"type\":\"basic\",\"username\":"} + json_quote(value.username)
               + ",\"password\":" + json_quote(value.password) + "}";
      } else if constexpr(std::is_same_v<T, ApiKeyAuth>) {
        const char* loc = value.location == ApiKeyAuth::Location::Query ? "query" : "header";
        return std::string{"{\"type\":\"apikey\",\"key\":"} + json_quote(value.key)
               + ",\"value\":" + json_quote(value.value) + ",\"location\":\"" + loc
               + "\",\"header_name\":" + json_quote(value.header_name) + "}";
      } else if constexpr(std::is_same_v<T, OAuth2ClientCredentialsAuth>) {
        return std::string{"{\"type\":\"oauth2_client_credentials\",\"token_url\":"}
               + json_quote(value.token_url) + ",\"client_id\":" + json_quote(value.client_id)
               + ",\"client_secret\":" + json_quote(value.client_secret) + ",\"scope\":"
               + json_quote(value.scope) + ",\"access_token\":" + json_quote(value.access_token)
               + ",\"expires_at\":" + std::to_string(value.expires_at) + "}";
      }
    },
    auth);
}

//----------------------------------------
//
//----------------------------------------
std::expected<AuthConfig, Error> deserialize_auth(std::string_view json) {
  std::string type;
  if(not extract_json_string(json, "type", type)) {
    return NoAuth{};
  }
  if(type == "none") {
    return NoAuth{};
  }
  if(type == "bearer") {
    BearerAuth auth;
    (void)extract_json_string(json, "token", auth.token);
    return auth;
  }
  if(type == "basic") {
    BasicAuth auth;
    (void)extract_json_string(json, "username", auth.username);
    (void)extract_json_string(json, "password", auth.password);
    return auth;
  }
  if(type == "apikey") {
    ApiKeyAuth auth;
    (void)extract_json_string(json, "key", auth.key);
    (void)extract_json_string(json, "value", auth.value);
    (void)extract_json_string(json, "header_name", auth.header_name);
    std::string location;
    if(extract_json_string(json, "location", location) && location == "query") {
      auth.location = ApiKeyAuth::Location::Query;
    }
    return auth;
  }
  if(type == "oauth2_client_credentials") {
    OAuth2ClientCredentialsAuth auth;
    (void)extract_json_string(json, "token_url", auth.token_url);
    (void)extract_json_string(json, "client_id", auth.client_id);
    (void)extract_json_string(json, "client_secret", auth.client_secret);
    (void)extract_json_string(json, "scope", auth.scope);
    (void)extract_json_string(json, "access_token", auth.access_token);
    long long expires_at = 0;
    if(extract_json_int(json, "expires_at", expires_at)) {
      auth.expires_at = expires_at;
    }
    return auth;
  }
  return std::unexpected(make_error(ErrorKind::Serialization, "unknown auth type", type));
}

//----------------------------------------
//
//----------------------------------------
std::string serialize_settings(const RequestSettings& settings) {
  std::ostringstream oss;
  oss << '{' << "\"timeout_ms\":" << settings.timeout.count() << ','
      << "\"follow_redirects\":" << (settings.follow_redirects ? "true" : "false") << ','
      << "\"max_redirects\":" << settings.max_redirects << ','
      << "\"verify_peer\":" << (settings.tls.verify_peer ? "true" : "false") << ','
      << "\"verify_host\":" << (settings.tls.verify_host ? "true" : "false") << ','
      << "\"max_response_bytes\":" << settings.max_response_bytes << ','
      << "\"proxy_enabled\":" << (settings.proxy.enabled ? "true" : "false") << ','
      << "\"proxy_url\":" << json_quote(settings.proxy.url) << ','
      << "\"proxy_username\":" << json_quote(settings.proxy.username) << ','
      << "\"proxy_password\":" << json_quote(settings.proxy.password) << ','
      << "\"identity_preset\":" << json_quote(settings.identity.preset_id) << ','
      << "\"identity_impersonate\":" << json_quote(settings.identity.impersonate) << ','
      << "\"identity_user_agent\":" << json_quote(settings.identity.user_agent) << ','
      << "\"identity_browser_headers\":"
      << (settings.identity.apply_browser_headers ? "true" : "false") << '}';
  return oss.str();
}

//----------------------------------------
//
//----------------------------------------
std::expected<RequestSettings, Error> deserialize_settings(std::string_view json) {
  RequestSettings settings;
  long long timeout_ms = settings.timeout.count();
  if(extract_json_int(json, "timeout_ms", timeout_ms)) {
    settings.timeout = std::chrono::milliseconds{timeout_ms};
  }
  (void)extract_json_bool(json, "follow_redirects", settings.follow_redirects);
  long long max_redirects = settings.max_redirects;
  if(extract_json_int(json, "max_redirects", max_redirects)) {
    settings.max_redirects = static_cast<int>(max_redirects);
  }
  (void)extract_json_bool(json, "verify_peer", settings.tls.verify_peer);
  (void)extract_json_bool(json, "verify_host", settings.tls.verify_host);
  long long max_bytes = static_cast<long long>(settings.max_response_bytes);
  if(extract_json_int(json, "max_response_bytes", max_bytes)) {
    settings.max_response_bytes = static_cast<std::size_t>(max_bytes);
  }
  (void)extract_json_bool(json, "proxy_enabled", settings.proxy.enabled);
  (void)extract_json_string(json, "proxy_url", settings.proxy.url);
  (void)extract_json_string(json, "proxy_username", settings.proxy.username);
  (void)extract_json_string(json, "proxy_password", settings.proxy.password);
  (void)extract_json_string(json, "identity_preset", settings.identity.preset_id);
  (void)extract_json_string(json, "identity_impersonate", settings.identity.impersonate);
  (void)extract_json_string(json, "identity_user_agent", settings.identity.user_agent);
  (void)extract_json_bool(json, "identity_browser_headers",
                          settings.identity.apply_browser_headers);
  settings.identity = resolve_client_identity(settings.identity);
  return settings;
}

//----------------------------------------
//
//----------------------------------------
std::string serialize_request(const HttpRequest& request) {
  std::ostringstream oss;
  oss << '{' << "\"method\":" << json_quote(to_string(request.method)) << ','
      << "\"url\":" << json_quote(request.url) << ',' << "\"body\":" << json_quote(request.body)
      << ',' << "\"auth\":" << serialize_auth(request.auth) << ','
      << "\"settings\":" << serialize_settings(request.settings) << ',' << "\"headers\":[";
  for(std::size_t i = 0; i < request.headers.size(); ++i) {
    if(i != 0) {
      oss << ',';
    }
    oss << "{\"name\":" << json_quote(request.headers[i].name)
        << ",\"value\":" << json_quote(request.headers[i].value)
        << ",\"enabled\":" << (request.headers[i].enabled ? "true" : "false") << '}';
  }
  oss << "],\"query\":[";
  for(std::size_t i = 0; i < request.query.size(); ++i) {
    if(i != 0) {
      oss << ',';
    }
    oss << "{\"name\":" << json_quote(request.query[i].name)
        << ",\"value\":" << json_quote(request.query[i].value)
        << ",\"enabled\":" << (request.query[i].enabled ? "true" : "false") << '}';
  }
  oss << "],\"cookies\":[";
  for(std::size_t i = 0; i < request.cookies.size(); ++i) {
    if(i != 0) {
      oss << ',';
    }
    oss << "{\"name\":" << json_quote(request.cookies[i].name)
        << ",\"value\":" << json_quote(request.cookies[i].value)
        << ",\"enabled\":" << (request.cookies[i].enabled ? "true" : "false") << '}';
  }
  oss << "]}";
  return oss.str();
}

//----------------------------------------
//
//----------------------------------------
std::expected<HttpRequest, Error> deserialize_request(std::string_view json) {
  HttpRequest request;
  std::string method;
  if(extract_json_string(json, "method", method)) {
    HttpMethod parsed{};
    if(not try_parse_http_method(method, parsed)) {
      return std::unexpected(make_error(ErrorKind::Serialization, "invalid method", method));
    }
    request.method = parsed;
  }
  (void)extract_json_string(json, "url", request.url);
  (void)extract_json_string(json, "body", request.body);

  // Nested auth/settings: find by key and take balanced braces — approximate via substring search.
  auto auth_pos = json.find("\"auth\"");
  if(auth_pos != std::string_view::npos) {
    auto brace = json.find('{', auth_pos);
    if(brace != std::string_view::npos) {
      int depth = 0;
      for(std::size_t i = brace; i < json.size(); ++i) {
        if(json[i] == '{') {
          ++depth;
        } else if(json[i] == '}') {
          --depth;
          if(depth == 0) {
            auto auth = deserialize_auth(json.substr(brace, i - brace + 1));
            if(not auth) {
              return std::unexpected(auth.error());
            }
            request.auth = std::move(*auth);
            break;
          }
        }
      }
    }
  }

  auto settings_pos = json.find("\"settings\"");
  if(settings_pos != std::string_view::npos) {
    auto brace = json.find('{', settings_pos);
    if(brace != std::string_view::npos) {
      int depth = 0;
      for(std::size_t i = brace; i < json.size(); ++i) {
        if(json[i] == '{') {
          ++depth;
        } else if(json[i] == '}') {
          --depth;
          if(depth == 0) {
            auto settings = deserialize_settings(json.substr(brace, i - brace + 1));
            if(not settings) {
              return std::unexpected(settings.error());
            }
            request.settings = std::move(*settings);
            break;
          }
        }
      }
    }
  }

  parse_key_values(extract_object_array(json, "headers"), request.headers);
  parse_query(extract_object_array(json, "query"), request.query);

  auto cookies = extract_object_array(json, "cookies");
  std::size_t pos = 0;
  while(true) {
    auto start = cookies.find('{', pos);
    if(start == std::string_view::npos) {
      break;
    }
    auto end = cookies.find('}', start);
    if(end == std::string_view::npos) {
      break;
    }
    auto obj = cookies.substr(start, end - start + 1);
    HttpCookie cookie;
    (void)extract_json_string(obj, "name", cookie.name);
    (void)extract_json_string(obj, "value", cookie.value);
    bool enabled = true;
    (void)extract_json_bool(obj, "enabled", enabled);
    cookie.enabled = enabled;
    request.cookies.push_back(std::move(cookie));
    pos = end + 1;
  }

  return request;
}

} // namespace requestman
