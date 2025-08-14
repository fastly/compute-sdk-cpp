cmake_minimum_required(VERSION 3.15)
if(!EXAMPLE_NAME)
    set(EXAMPLE_NAME "echo")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TARGET_DIR "debug")
else ()
    set(TARGET_DIR "release")
endif ()

project(cxx_example)

add_link_options(-L${WASI_SDK_PREFIX}/share/wasi-sysroot/lib/wasm32-wasi)

add_subdirectory(.. ../../target)

add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)
target_compile_options(${EXAMPLE_NAME} PRIVATE -Wall -Wextra -Werror -fno-exceptions)

option(ENABLE_LTO "Enable cross language linking time optimization" OFF)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO / LTO enabled")
        set_target_properties(${EXAMPLE_NAME} PROPERTIES CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        target_link_options(${EXAMPLE_NAME} PRIVATE -fuse-ld=lld)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()

set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX .wasm)
target_include_directories(${EXAMPLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../../${TARGET_DIR})
target_link_libraries(${EXAMPLE_NAME} fastly::thin)
