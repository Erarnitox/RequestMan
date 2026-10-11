# Copies libcurl-impersonate shared objects into a binary directory.
if(NOT DEFINED src_dir OR NOT DEFINED dst_dir)
  message(FATAL_ERROR "src_dir and dst_dir are required")
endif()

file(GLOB _libs
  "${src_dir}/libcurl-impersonate.so"
  "${src_dir}/libcurl-impersonate.so.*"
)
foreach(_lib IN LISTS _libs)
  get_filename_component(_name "${_lib}" NAME)
  # Skip the tiny runtime-probe helper if present.
  if(_name MATCHES "runtime-probe")
    continue()
  endif()
  file(COPY "${_lib}" DESTINATION "${dst_dir}" FOLLOW_SYMLINK_CHAIN)
endforeach()
