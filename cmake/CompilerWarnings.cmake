# Strong warnings for first-party targets; never force Werror on third-party code.
function(requestman_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /WX)
  else()
    target_compile_options(${target} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Wcast-align
      -Wunused
      -Woverloaded-virtual
      -Wnull-dereference
      -Wformat=2
      -Werror
    )
  endif()

  # Apply clang-tidy only to first-party targets (never Catch2/raylib/etc.).
  if(REQUESTMAN_ENABLE_CLANG_TIDY)
    set_target_properties(${target} PROPERTIES
      CXX_CLANG_TIDY "clang-tidy;--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
    )
  endif()
endfunction()
