file(GLOB_RECURSE TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(t ${TEST_SRC})
    get_filename_component(testname ${t} NAME_WE)
    add_executable(${testname} ${t})
    set_property(TARGET ${testname} PROPERTY
        TEST_LAUNCHER viceroy run -C fastly.toml)
    target_link_libraries(${testname} PRIVATE fastly::thin Catch2::Catch2)
    target_include_directories(${testname} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include)
    target_compile_options(${testname} PRIVATE -Wall -Wextra -Werror ${FASTLY_CXXFLAGS})
    add_test(NAME ${testname}
        COMMAND ${testname}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
