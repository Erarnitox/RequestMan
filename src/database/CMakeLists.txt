add_library(requestman_database
  sqlite_db.cpp
  request_json.cpp
  sqlite_repositories.cpp
)
add_library(requestman::database ALIAS requestman_database)

target_include_directories(requestman_database PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(requestman_database
  PUBLIC requestman_domain
  PRIVATE SQLite3::SQLite3
)

target_compile_features(requestman_database PUBLIC cxx_std_23)
requestman_set_warnings(requestman_database)
requestman_enable_sanitizers(requestman_database)
