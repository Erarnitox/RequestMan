cmake_minimum_required(VERSION 4.0)

project(RequestMan
  VERSION 0.1.0
  DESCRIPTION "Native cross-platform HTTP API client"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(REQUESTMAN_BUILD_TESTS "Build unit and integration tests" ON)
option(REQUESTMAN_BUILD_UI "Build Raylib/Raygui desktop UI" ON)
option(REQUESTMAN_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(REQUESTMAN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(REQUESTMAN_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(REQUESTMAN_ENABLE_CLANG_TIDY "Run clang-tidy on first-party targets" OFF)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  option(REQUESTMAN_WITH_CURL_IMPERSONATE
         "Link lexiforest libcurl-impersonate for real browser TLS fingerprints" ON)
else()
  option(REQUESTMAN_WITH_CURL_IMPERSONATE
         "Link lexiforest libcurl-impersonate for real browser TLS fingerprints" OFF)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CPM)
include(CompilerWarnings)
include(Sanitizers)

# Prefer system packages when available; fall back via CPM only when needed.
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

if(REQUESTMAN_WITH_CURL_IMPERSONATE)
  include(FetchCurlImpersonate)
  requestman_setup_curl_impersonate()
  set(REQUESTMAN_CURL_TARGET CurlImpersonate::libcurl)
else()
  find_package(CURL REQUIRED)
  set(REQUESTMAN_CURL_TARGET CURL::libcurl)
endif()

add_subdirectory(src/domain)
add_subdirectory(src/logging)
add_subdirectory(src/platform)
add_subdirectory(src/http)
add_subdirectory(src/database)
add_subdirectory(src/application)

if(REQUESTMAN_BUILD_UI)
  add_subdirectory(src/ui)
  add_executable(requestman src/main.cpp)
  target_link_libraries(requestman PRIVATE
    requestman_ui
    requestman_application
    requestman_platform
    requestman_logging
  )
  requestman_set_warnings(requestman)
  requestman_enable_sanitizers(requestman)

  add_custom_command(TARGET requestman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:requestman>/assets"
    COMMENT "Copy UI assets next to requestman"
  )

  if(REQUESTMAN_WITH_CURL_IMPERSONATE)
    requestman_copy_curl_impersonate_runtime(requestman)
  endif()
endif()

if(REQUESTMAN_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
