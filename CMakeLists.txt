# From https://github.com/XiangpengHao/cxx-cmake-example/blob/master/fastly/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(compute-sdk-cpp)

if(!WASI_SDK)
    set(WASI_SDK "/opt/wasi-sdk")
endif()

if(VERBOSE)
    set(VERBOSE_FLAG "--verbose")
endif()

set(CMAKE_CXX_STANDARD 20)

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

option(ENABLE_LTO "Enable cross language linking time optimization" ON)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        add_link_options(-fuse-ld=lld)
        set(CFLAGS "-flto=thin -Oz")
        set(CXXFLAGS "${CMAKE_CXX_FLAGS} -flto=thin -Oz --sysroot=${WASI_SDK}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions")
        set(CMAKE_CXX_FLAGS ${CXXFLAGS})
        set(RUST_FLAGS "-Clinker-plugin-lto" "-Clinker=${CMAKE_CURRENT_SOURCE_DIR}/build-workaround.sh" "-Clink-arg=-fuse-ld=lld" "-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
        set(CXXFLAGS "${CMAKE_CXX_FLAGS} --sysroot=${WASI_SDK}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions")
        set(CMAKE_CXX_FLAGS ${CXXFLAGS})
        set(RUST_FLAGS "-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
    endif()
else()
    set(CXXFLAGS "${CMAKE_CXX_FLAGS} --sysroot=${WASI_SDK}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions")
    set(CMAKE_CXX_FLAGS ${CXXFLAGS})
    set(RUST_FLAGS "-L${WASI_SDK}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
endif()
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()

set(FASTLY_SYS_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_TRIPLE}/${TARGET_DIR}/libfastly_sys.a")

set(FASTLY_SYS_CXX "${CMAKE_CURRENT_BINARY_DIR}/fastly-sys.cpp")
file(GLOB_RECURSE FASTLY_CXX "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/**/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/*.cpp")

add_library(fastly-sys STATIC ${FASTLY_SYS_CXX})
add_library(fastly-sys-orig STATIC IMPORTED)
add_library(fastly-thin STATIC ${FASTLY_CXX} fastly-sys ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tl/expected.h)
set_target_properties(fastly-sys-orig PROPERTIES IMPORTED_LOCATION ${FASTLY_SYS_LIB})

if(MSVC)
    target_compile_options(fastly-thin PRIVATE /W3 /WX)
else()
    target_compile_options(fastly-thin PRIVATE -Wall -Wextra -Werror)
endif()

set_target_properties(
    fastly-sys fastly-thin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}
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

target_include_directories(fastly-thin PUBLIC ${DIST_DIR} ${DIST_DIR}/fastly ${DIST_DIR}/fastly/http)

target_link_libraries(fastly-thin PUBLIC fastly-sys-orig fastly-sys)

function(bundle_static_library tgt_name bundled_tgt_name)
    list(APPEND static_libs ${tgt_name})

    function(_recursively_collect_dependencies input_target)
        set(_input_link_libraries LINK_LIBRARIES)
        get_target_property(_input_type ${input_target} TYPE)
        if (${_input_type} STREQUAL "INTERFACE_LIBRARY")
            set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
        endif()
        get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
        foreach(dependency IN LISTS public_dependencies)
            if(TARGET ${dependency})
                get_target_property(alias ${dependency} ALIASED_TARGET)
                if (TARGET ${alias})
                    set(dependency ${alias})
                endif()
                get_target_property(_type ${dependency} TYPE)
                if (${_type} STREQUAL "STATIC_LIBRARY")
                    list(APPEND static_libs ${dependency})
                endif()

                get_property(library_already_added
          GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency})
                if (NOT library_already_added)
                    set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
                    _recursively_collect_dependencies(${dependency})
                endif()
            endif()
        endforeach()
        set(static_libs ${static_libs} PARENT_SCOPE)
    endfunction()

    _recursively_collect_dependencies(${tgt_name})

    list(REMOVE_DUPLICATES static_libs)

    set(bundled_tgt_full_name
    ${DIST_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

    if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
      "CREATE ${bundled_tgt_full_name}\n" )

        foreach(tgt IN LISTS static_libs)
            file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
        "ADDLIB $<TARGET_FILE:${tgt}>\n")
        endforeach()

        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

        file(GENERATE
      OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

        set(ar_tool ${CMAKE_AR})
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        add_custom_command(
      COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM)
    elseif(MSVC)
        find_program(lib_tool lib)

        foreach(tgt IN LISTS static_libs)
            list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
        endforeach()

        add_custom_command(
      COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM)
    else()
        message(FATAL_ERROR "Unknown bundle scenario!")
    endif()

    add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
    add_dependencies(bundling_target ${tgt_name})

    add_library(${bundled_tgt_name} STATIC IMPORTED)
    set_target_properties(${bundled_tgt_name} ${bundled_target_full_name}
    PROPERTIES
      IMPORTED_LOCATION ${bundled_tgt_full_name}
      INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>
      LIBRARY_OUTPUT_DIRECTORY ${DIST_DIR}
    )
    add_dependencies(${bundled_tgt_name} bundling_target)

endfunction()
bundle_static_library(fastly-thin fastly)

file(GLOB_RECURSE FASTLY_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/**/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/*.h")
foreach(f ${FASTLY_HEADERS})
    file(RELATIVE_PATH fr "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp" ${f})
    configure_file(${f} ${DIST_DIR}/fastly/${fr} COPYONLY)
    list(APPEND incls "#include \"${fr}\"\n")
endforeach(f)
list(APPEND incls "#include \"expected.h\"\n")
file(WRITE ${DIST_DIR}/fastly/sdk.h ${incls})

add_test(NAME fastly_test
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
