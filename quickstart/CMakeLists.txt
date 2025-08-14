cmake_minimum_required(VERSION 3.15)
project(quickstart)

if("${WASI_SDK_PREFIX}" STREQUAL "")
    message(FATAL_ERROR "You must supply a CMAKE_TOOLCHAIN_FILE argument to CMake, pointing to the wasi-sdk-p1.cmake file in your WASI SDK installation.")
endif()

find_package(fastly REQUIRED)

add_executable(main main.cpp)

option(ENABLE_LTO "Enable cross language linking time optimization" ON)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO / LTO enabled")
        set_target_properties(main PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        target_link_options(main PRIVATE -fuse-ld=lld)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()

target_compile_options(main PRIVATE -Wall -Wextra -Werror -fno-exceptions)
target_compile_features(main PRIVATE cxx_std_20)
set_target_properties(main PROPERTIES SUFFIX .wasm)
target_link_libraries(main PRIVATE fastly::fastly)
target_link_options(main PRIVATE "-L${WASI_SDK_PREFIX}/share/wasi-sysroot/lib/wasm32-wasi")
