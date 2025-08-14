# From https://github.com/XiangpengHao/cxx-cmake-example/blob/master/fastly/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

project(compute-sdk-cpp VERSION 0.2.0 LANGUAGES CXX)

if(VERBOSE)
    set(VERBOSE_FLAG "--verbose")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build --target=${CMAKE_CXX_COMPILER_TARGET} ${VERBOSE_FLAG})
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build --target=${CMAKE_CXX_COMPILER_TARGET} --release ${VERBOSE_FLAG})
    set(TARGET_DIR "release")
endif ()

option(ENABLE_LTO "Enable cross language linking time optimization" ON)
set(LTO_SUPPORTED FALSE)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO / LTO enabled")
        set(LTO_SUPPORTED TRUE)
        set(FASTLY_LDFLAGS -fuse-ld=lld)
        set(FASTLY_CFLAGS -flto=thin -Oz)
        set(FASTLY_CXXFLAGS -flto=thin -Oz "--sysroot=${WASI_SDK_PREFIX}/share/wasi-sysroot" -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions)
        set(FASTLY_RUST_FLAGS -Clinker-plugin-lto "-Clinker=${CMAKE_CURRENT_SOURCE_DIR}/build-workaround.sh" "-Clink-arg=-fuse-ld=lld" "-L${WASI_SDK_PREFIX}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
        set(FASTLY_CXXFLAGS "--sysroot=${WASI_SDK_PREFIX}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions")
        set(FASTLY_RUST_FLAGS "-L${WASI_SDK_PREFIX}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
    endif()
else()
    set(FASTLY_CXXFLAGS "-sysroot=${WASI_SDK_PREFIX}/share/wasi-sysroot -DRUST_CXX_NO_EXCEPTIONS -fno-exceptions")
    set(FASTLY_RUST_FLAGS "-L${WASI_SDK_PREFIX}/share/wasi-sysroot/lib/wasm32-wasi" "-lstatic=c++" "-lstatic=c++abi")
endif()

set(FASTLY_SYS_LIB "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CXX_COMPILER_TARGET}/${TARGET_DIR}/libfastly_sys.a")

set(FASTLY_SYS_CXX "${CMAKE_CURRENT_BINARY_DIR}/fastly-sys.cpp")
file(GLOB_RECURSE FASTLY_CXX "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/**/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/*.cpp")

add_library(fastly-sys STATIC ${FASTLY_SYS_CXX})
add_library(fastly::sys ALIAS fastly-sys)
set_target_properties(fastly-sys PROPERTIES 
    INTERPROCEDURAL_OPTIMIZATION ${LTO_SUPPORTED})
target_link_options(fastly-sys PRIVATE ${FASTLY_LDFLAGS})

add_library(fastly-sys-orig STATIC IMPORTED)
set_target_properties(fastly-sys-orig PROPERTIES IMPORTED_LOCATION ${FASTLY_SYS_LIB})

add_library(fastly-thin STATIC ${FASTLY_CXX})
add_library(fastly::thin ALIAS fastly-thin)
target_include_directories(fastly-thin PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tl)
set_target_properties(fastly-thin PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ${LTO_SUPPORTED})
target_link_options(fastly-thin PRIVATE ${FASTLY_LDFLAGS})

target_compile_options(fastly-thin PRIVATE -Wall -Wextra -Werror ${FASTLY_CXXFLAGS})
target_compile_options(fastly-sys PRIVATE ${FASTLY_CXXFLAGS})
target_compile_features(fastly-thin PUBLIC cxx_std_20)

set_target_properties(
    fastly-sys fastly-thin
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}
)

file(GLOB_RECURSE RUST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.rs" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs")

add_custom_command(
    OUTPUT ${FASTLY_SYS_CXX} ${FASTLY_SYS_LIB} ${CMAKE_CURRENT_BINARY_DIR}/include/fastly/sdk-sys.h
    COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} CC="${CMAKE_C_COMPILER}" CXX="${CMAKE_CXX_COMPILER}" CFLAGS="${FASTLY_CFLAGS}" CXXFLAGS="${FASTLY_CXXFLAGS}" RUSTFLAGS="${FASTLY_RUST_FLAGS}" ${CARGO_CMD}
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fastly-sys/src/lib.rs.cc ${FASTLY_SYS_CXX}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/fastly
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/fastly-sys/src/lib.rs.h ${CMAKE_CURRENT_BINARY_DIR}/include/fastly/sdk-sys.h
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tl/expected.h ${CMAKE_CURRENT_BINARY_DIR}/include/fastly/expected.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${RUST_SRC}
)

target_include_directories(fastly-thin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include)

target_link_libraries(fastly-thin PUBLIC fastly-sys-orig fastly::sys)

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

    set(bundled_tgt_full_name "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX}" CACHE INTERNAL "Bundled target full name")

    if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar.in "CREATE ${bundled_tgt_full_name}\n" )

        foreach(tgt IN LISTS static_libs)
            file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar.in
                "ADDLIB $<TARGET_FILE:${tgt}>\n")
        endforeach()

        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

        file(GENERATE
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar
            INPUT ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar.in)

        set(ar_tool ${CMAKE_AR})
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        add_custom_command(
            COMMAND ${ar_tool} -M < ${CMAKE_CURRENT_BINARY_DIR}/${bundled_tgt_name}.ar
            OUTPUT ${bundled_tgt_full_name}
            COMMENT "Bundling ${bundled_tgt_name}"
            VERBATIM)
    else()
        message(FATAL_ERROR "Unknown bundle scenario!")
    endif()

    add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
    add_dependencies(bundling_target ${tgt_name})

    add_library(${bundled_tgt_name} STATIC IMPORTED)
    set_target_properties(${bundled_tgt_name} PROPERTIES
        IMPORTED_LOCATION ${bundled_tgt_full_name}
        INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_dependencies(${bundled_tgt_name} bundling_target)

endfunction()
bundle_static_library(fastly-thin fastly)

file(GLOB_RECURSE FASTLY_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h")
foreach(f ${FASTLY_HEADERS})
    file(RELATIVE_PATH fr "${CMAKE_CURRENT_SOURCE_DIR}/include" ${f})
    list(APPEND incls "#include <${fr}>\n")
endforeach(f)
list(APPEND incls "#include <fastly/expected.h>\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/fastly/sdk.h ${incls})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if (BUILD_TESTING)
    add_test(NAME fastly_test
        COMMAND cargo test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    fastly-config-version.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

include(GNUInstallDirs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fastly-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fastly
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fastly-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fastly
)

install(FILES ${bundled_tgt_full_name}
    DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/fastly
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/fastly
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


