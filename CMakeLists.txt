# From https://github.com/XiangpengHao/cxx-cmake-example/blob/master/fastly/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(compute-sdk-cpp)

if(!WASI_SDK)
    set(WASI_SDK "/opt/wasi-sdk")
endif()

if(VERBOSE)
    set(VERBOSE_FLAG "--verbose")
endif()

set(CMAKE_CXX_STANDARD 17)

set(TARGET_TRIPLE wasm32-wasip1)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build --target=${TARGET_TRIPLE} ${VERBOSE_FLAG})
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --target=${TARGET_TRIPLE} --release ${VERBOSE_FLAG})
    set(TARGET_DIR "release")
endif ()

set(DIST_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/dist)

set(CC "${WASI_SDK}/bin/clang")
set(ENV{CC} "${WASI_SDK}/bin/clang")
set(CXX "${WASI_SDK}/bin/clang++")

if(ENABLE_LTO)
    set(CFLAGS "-flto=thin -Oz")
    set(CXXFLAGS "-flto=thin -Oz --sysroot=${WASI_SDK}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS")
    set(RUST_FLAGS "-Clinker-plugin-lto" "-Clinker=${CMAKE_CURRENT_SOURCE_DIR}/build-workaround.sh" "-Clink-arg=-fuse-ld=lld" "-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
else()
    set(CXXFLAGS "--sysroot=${WASI_SDK}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS")
    set(RUST_FLAGS "-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
endif()

set(FASTLY_SYS_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_TRIPLE}/${TARGET_DIR}/libfastly_sys.a")

set(FASTLY_SYS_CXX "${CMAKE_CURRENT_BINARY_DIR}/fastly-sys.cpp")
file(GLOB_RECURSE FASTLY_CXX "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/**/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/*.cpp")

add_library(fastly-sys STATIC ${FASTLY_SYS_CXX})
add_library(fastly STATIC ${FASTLY_CXX} fastly-sys)

# TODO(@zkat): sigh. Let's do this later. Can't do pedantic, though.
# if(MSVC)
#     target_compile_options(fastly PRIVATE /W4 /WX)
# else()
#     target_compile_options(fastly PRIVATE -Wall -Wextra -Werror)
# endif()

set_target_properties(
    fastly-sys fastly
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${DIST_DIR}
)

file(GLOB_RECURSE RUST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.rs" "${CMAKE_CURRENT_COURSE_DIR}/src/*.rs")

add_custom_command(
    OUTPUT ${FASTLY_SYS_CXX} ${FASTLY_SYS_LIB} ${DIST_DIR}/fastly/sdk-sys.h ${DIST_DIR}/fastly/util.h
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} CC=${CC} CXX=${CXX} CFLAGS=${CFLAGS} CXXFLAGS=${CXXFLAGS} RUSTFLAGS="${RUST_FLAGS}" ${CARGO_CMD}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fastly-sys/src/lib.rs.cc ${FASTLY_SYS_CXX}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fastly-sys/src/lib.rs.h ${DIST_DIR}/fastly/sdk-sys.h
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tl/expected.h ${DIST_DIR}/fastly/expected.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${RUST_SRC}
)

file(GLOB_RECURSE FASTLY_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/**/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/vendor/**/*.h")
foreach(f ${FASTLY_HEADERS})
    file(RELATIVE_PATH fr "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp" ${f})
    configure_file(${f} ${DIST_DIR}/fastly/${fr} COPYONLY)
    list(APPEND incls "#include \"${fr}\"\n")
endforeach()
file(WRITE ${DIST_DIR}/fastly/sdk.h ${incls})

target_include_directories(fastly PUBLIC ${DIST_DIR} ${DIST_DIR}/fastly ${DIST_DIR}/fastly/http)

target_link_libraries(fastly ${FASTLY_SYS_LIB})

add_test(NAME fastly_test
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
