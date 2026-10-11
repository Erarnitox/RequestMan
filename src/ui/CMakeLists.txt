include(CPM)

# raylib (graphics) + raygui (immediate-mode widgets)
CPMAddPackage(
  NAME raylib
  GITHUB_REPOSITORY raysan5/raylib
  GIT_TAG 5.5
  OPTIONS
    "BUILD_EXAMPLES OFF"
    "BUILD_GAMES OFF"
)

CPMAddPackage(
  NAME raygui
  GITHUB_REPOSITORY raysan5/raygui
  GIT_TAG 4.0
)

# Compile raygui implementation without -Werror (third-party source).
add_library(requestman_raygui STATIC raygui_impl.cpp)
target_include_directories(requestman_raygui PUBLIC ${raygui_SOURCE_DIR}/src)
target_link_libraries(requestman_raygui PUBLIC raylib)
target_compile_features(requestman_raygui PUBLIC cxx_std_23)

add_library(requestman_ui
  main_window.cpp
  code_view.cpp
  syntax_lexers.cpp
  theme.cpp
  icons.cpp
)
add_library(requestman::ui ALIAS requestman_ui)

target_include_directories(requestman_ui PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${raygui_SOURCE_DIR}/src
)

target_link_libraries(requestman_ui
  PUBLIC
    requestman_application
    requestman_domain
    requestman_raygui
    requestman_logging
)

target_compile_features(requestman_ui PUBLIC cxx_std_23)
target_compile_definitions(requestman_ui
  PRIVATE "REQUESTMAN_VERSION=\"${PROJECT_VERSION}\""
)
requestman_set_warnings(requestman_ui)
requestman_enable_sanitizers(requestman_ui)
