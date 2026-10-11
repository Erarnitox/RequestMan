# Locates or downloads lexiforest libcurl-impersonate and exposes CurlImpersonate::libcurl.

set(REQUESTMAN_CURL_IMPERSONATE_VERSION "v2.2.2" CACHE STRING
    "lexiforest/curl-impersonate release tag")

function(requestman_resolve_curl_impersonate_triplet out_triplet out_asset)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(${out_triplet} "x86_64-linux-gnu" PARENT_SCOPE)
    set(${out_asset}
        "libcurl-impersonate-${REQUESTMAN_CURL_IMPERSONATE_VERSION}.x86_64-linux-gnu.tar.gz"
        PARENT_SCOPE)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(${out_triplet} "aarch64-linux-gnu" PARENT_SCOPE)
    set(${out_asset}
        "libcurl-impersonate-${REQUESTMAN_CURL_IMPERSONATE_VERSION}.aarch64-linux-gnu.tar.gz"
        PARENT_SCOPE)
  else()
    set(${out_triplet} "" PARENT_SCOPE)
    set(${out_asset} "" PARENT_SCOPE)
  endif()
endfunction()

function(requestman_setup_curl_impersonate)
  if(TARGET CurlImpersonate::libcurl)
    return()
  endif()

  set(_root "")
  if(DEFINED CURL_IMPERSONATE_ROOT AND NOT CURL_IMPERSONATE_ROOT STREQUAL "")
    set(_root "${CURL_IMPERSONATE_ROOT}")
  elseif(DEFINED ENV{CURL_IMPERSONATE_ROOT} AND NOT "$ENV{CURL_IMPERSONATE_ROOT}" STREQUAL "")
    set(_root "$ENV{CURL_IMPERSONATE_ROOT}")
  endif()

  set(_include_dir "")
  set(_lib_dir "")
  set(_lib_file "")

  if(_root)
    if(EXISTS "${_root}/include/curl/curl.h")
      set(_include_dir "${_root}/include")
    elseif(EXISTS "${_root}/curl/curl.h")
      set(_include_dir "${_root}")
    endif()
    foreach(_candidate
            "${_root}/lib/libcurl-impersonate.so"
            "${_root}/lib64/libcurl-impersonate.so"
            "${_root}/libcurl-impersonate.so")
      if(EXISTS "${_candidate}")
        set(_lib_file "${_candidate}")
        get_filename_component(_lib_dir "${_candidate}" DIRECTORY)
        break()
      endif()
    endforeach()
  endif()

  if(NOT _lib_file)
    requestman_resolve_curl_impersonate_triplet(_triplet _asset)
    if(_asset STREQUAL "")
      message(FATAL_ERROR
        "REQUESTMAN_WITH_CURL_IMPERSONATE is ON but no prebuilt libcurl-impersonate "
        "is available for ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}. "
        "Install libcurl-impersonate and set CURL_IMPERSONATE_ROOT, or disable "
        "REQUESTMAN_WITH_CURL_IMPERSONATE.")
    endif()

    set(_prefix "${CMAKE_BINARY_DIR}/_deps/curl-impersonate-${REQUESTMAN_CURL_IMPERSONATE_VERSION}")
    set(_archive "${_prefix}/${_asset}")
    set(_url
      "https://github.com/lexiforest/curl-impersonate/releases/download/${REQUESTMAN_CURL_IMPERSONATE_VERSION}/${_asset}")

    if(NOT EXISTS "${_prefix}/libcurl-impersonate.so" AND NOT EXISTS "${_prefix}/lib/libcurl-impersonate.so")
      message(STATUS "Downloading libcurl-impersonate ${_asset}")
      file(MAKE_DIRECTORY "${_prefix}")
      file(DOWNLOAD "${_url}" "${_archive}"
           SHOW_PROGRESS
           STATUS _dl_status
           TLS_VERIFY ON)
      list(GET _dl_status 0 _dl_code)
      if(NOT _dl_code EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(FATAL_ERROR "Failed to download libcurl-impersonate: ${_dl_msg}")
      endif()
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${_archive}"
        WORKING_DIRECTORY "${_prefix}"
        RESULT_VARIABLE _extract_rc)
      if(NOT _extract_rc EQUAL 0)
        message(FATAL_ERROR "Failed to extract libcurl-impersonate archive")
      endif()
    endif()

    if(EXISTS "${_prefix}/include/curl/curl.h")
      set(_include_dir "${_prefix}/include")
    endif()
    foreach(_candidate
            "${_prefix}/libcurl-impersonate.so"
            "${_prefix}/lib/libcurl-impersonate.so")
      if(EXISTS "${_candidate}")
        set(_lib_file "${_candidate}")
        get_filename_component(_lib_dir "${_candidate}" DIRECTORY)
        break()
      endif()
    endforeach()
  endif()

  if(NOT _lib_file OR NOT _include_dir)
    message(FATAL_ERROR
      "Could not locate libcurl-impersonate library/headers. "
      "Set CURL_IMPERSONATE_ROOT to an install prefix containing "
      "include/curl and libcurl-impersonate.so")
  endif()

  add_library(CurlImpersonate::libcurl SHARED IMPORTED GLOBAL)
  set_target_properties(CurlImpersonate::libcurl PROPERTIES
    IMPORTED_LOCATION "${_lib_file}"
    INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
  )

  set(REQUESTMAN_CURL_IMPERSONATE_LIB_DIR "${_lib_dir}" CACHE INTERNAL
      "Directory containing libcurl-impersonate shared libraries")
  set(REQUESTMAN_CURL_IMPERSONATE_LIB_FILE "${_lib_file}" CACHE INTERNAL
      "Path to libcurl-impersonate.so")

  message(STATUS "Using libcurl-impersonate: ${_lib_file}")
endfunction()

# Copy shared library (and SONAME links) next to a runtime binary.
function(requestman_copy_curl_impersonate_runtime target_name)
  if(NOT REQUESTMAN_WITH_CURL_IMPERSONATE)
    return()
  endif()
  if(NOT REQUESTMAN_CURL_IMPERSONATE_LIB_DIR)
    return()
  endif()
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${REQUESTMAN_CURL_IMPERSONATE_LIB_DIR}"
            "$<TARGET_FILE_DIR:${target_name}>/curl-impersonate-runtime"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${REQUESTMAN_CURL_IMPERSONATE_LIB_FILE}"
            "$<TARGET_FILE_DIR:${target_name}>/libcurl-impersonate.so"
    COMMAND ${CMAKE_COMMAND}
            -Dsrc_dir=${REQUESTMAN_CURL_IMPERSONATE_LIB_DIR}
            -Ddst_dir=$<TARGET_FILE_DIR:${target_name}>
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyCurlImpersonateRuntime.cmake
    COMMENT "Copy libcurl-impersonate runtime for ${target_name}"
  )
  set_property(TARGET ${target_name} APPEND PROPERTY BUILD_RPATH "$ORIGIN")
  set_property(TARGET ${target_name} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
endfunction()
