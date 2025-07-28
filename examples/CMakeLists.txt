cmake_minimum_required(VERSION 3.15)
if(!WASI_SDK)
    set(WASI_SDK "/opt/wasi-sdk")
endif()
if(!EXAMPLE_NAME)
    set(EXAMPLE_NAME "echo")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TARGET_DIR "debug")
else ()
    set(TARGET_DIR "release")
endif ()

set(CMAKE_C_COMPILER "${WASI_SDK}/bin/clang")
set(CMAKE_CXX_COMPILER "${WASI_SDK}/bin/clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

option(ENABLE_LTO "Enable cross language linking time optimization" OFF)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        add_link_options(-fuse-ld=lld)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()

project(cxx_example)

set (CMAKE_CXX_STANDARD 20)

add_link_options(-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi)

add_subdirectory(.. ../../target)

add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX .wasm)
target_include_directories(${EXAMPLE_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../../${TARGET_DIR})
target_link_libraries(${EXAMPLE_NAME} fastly-thin)
