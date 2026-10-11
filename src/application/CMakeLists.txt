add_library(requestman_application
  request_executor.cpp
  app_controller.cpp
)
add_library(requestman::application ALIAS requestman_application)

target_include_directories(requestman_application PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(requestman_application
  PUBLIC
    requestman_domain
    requestman_http
    requestman_database
    requestman_logging
  PRIVATE
    Threads::Threads
)

target_compile_features(requestman_application PUBLIC cxx_std_23)
requestman_set_warnings(requestman_application)
requestman_enable_sanitizers(requestman_application)
