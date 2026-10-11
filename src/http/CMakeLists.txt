add_library(requestman_http
  curl_http_client.cpp
  curl_identity.cpp
  curl_support.cpp
  oauth2_token.cpp
)
add_library(requestman::http ALIAS requestman_http)

target_include_directories(requestman_http PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(requestman_http
  PUBLIC requestman_domain
  PRIVATE ${REQUESTMAN_CURL_TARGET} requestman_logging
)

if(REQUESTMAN_WITH_CURL_IMPERSONATE)
  target_compile_definitions(requestman_http PUBLIC REQUESTMAN_HAS_CURL_IMPERSONATE=1)
else()
  target_compile_definitions(requestman_http PUBLIC REQUESTMAN_HAS_CURL_IMPERSONATE=0)
endif()

target_compile_features(requestman_http PUBLIC cxx_std_23)
requestman_set_warnings(requestman_http)
requestman_enable_sanitizers(requestman_http)
