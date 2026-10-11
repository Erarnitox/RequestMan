add_library(requestman_domain
  auth_apply.cpp
  body_format.cpp
  browser_presets.cpp
  json_util.cpp
  oauth2.cpp
  url_util.cpp
)

add_library(requestman::domain ALIAS requestman_domain)

target_include_directories(requestman_domain PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(requestman_domain PUBLIC cxx_std_23)
requestman_set_warnings(requestman_domain)
requestman_enable_sanitizers(requestman_domain)
