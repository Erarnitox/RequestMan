include(CPM)

CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  VERSION 3.8.0
)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(requestman_tests
  test_http_method.cpp
  test_auth_apply.cpp
  test_browser_presets.cpp
  test_body_format.cpp
  test_sqlite_repositories.cpp
  test_curl_http_client.cpp
  test_logging.cpp
  test_app_controller.cpp
  test_syntax_lexers.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/syntax_lexers.cpp
)

target_include_directories(requestman_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/ui)

target_link_libraries(requestman_tests PRIVATE
  Catch2::Catch2WithMain
  requestman_domain
  requestman_http
  requestman_database
  requestman_application
  requestman_platform
  requestman_logging
)

target_compile_features(requestman_tests PRIVATE cxx_std_23)
requestman_set_warnings(requestman_tests)
# Catch2 TEST_CASE macros use __COUNTER__, which Clang 22+ flags as a C2y extension under -Wpedantic/-Werror.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(requestman_tests PRIVATE -Wno-c2y-extensions)
endif()
requestman_enable_sanitizers(requestman_tests)

catch_discover_tests(requestman_tests)

if(REQUESTMAN_WITH_CURL_IMPERSONATE)
  requestman_copy_curl_impersonate_runtime(requestman_tests)
endif()
