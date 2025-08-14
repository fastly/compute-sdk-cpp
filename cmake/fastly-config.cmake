include(${CMAKE_CURRENT_LIST_DIR}/fastly-config-version.cmake)

add_library(fastly::fastly STATIC IMPORTED)

set_target_properties(fastly::fastly PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/../../libfastly.a"
  INTERFACE_COMPILE_FEATURES "cxx_std_20"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../include"
)

set(FASTLY_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../include")