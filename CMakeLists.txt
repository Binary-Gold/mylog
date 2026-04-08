cmake_minimum_required(VERSION 3.20)
project(myLog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MYLOG_BUILD_TESTS "Build test executables" ON)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE ALL_SOURCES
    CONFIGURE_DEPENDS
    "${ROOT_DIR}/*.cc"
    "${ROOT_DIR}/*.cpp"
)

list(FILTER ALL_SOURCES EXCLUDE REGEX [[/build/]])
list(FILTER ALL_SOURCES EXCLUDE REGEX [[/\\.git/]])

set(LIB_SOURCES "")
set(TEST_MAIN_SOURCES "")

foreach(src IN LISTS ALL_SOURCES)
    file(RELATIVE_PATH rel_path "${ROOT_DIR}" "${src}")
    get_filename_component(fname "${src}" NAME)

    # 约定：
    # - 非 test/、tests/ 下的源码 -> 归入库 mylog
    # - test/**/main.cpp / tests/**/main.cpp -> 生成可执行
    # - (^|.*/)(test|tests)/ 匹配顶层 test/... 与 foo/test/...
    if(rel_path MATCHES "(^|.*/)(test|tests)/" AND fname STREQUAL "main.cpp")
        list(APPEND TEST_MAIN_SOURCES "${src}")
    elseif(NOT rel_path MATCHES "(^|.*/)(test|tests)/")
        list(APPEND LIB_SOURCES "${src}")
    endif()
endforeach()

if(LIB_SOURCES STREQUAL "")
    message(FATAL_ERROR "No library sources found. Expected *.cc/*.cpp outside test/ or tests/.")
endif()

add_library(mylog STATIC ${LIB_SOURCES})
target_include_directories(mylog PUBLIC "${ROOT_DIR}")

if(NOT MSVC)
    target_compile_options(mylog PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MYLOG_BUILD_TESTS)
    foreach(main_src IN LISTS TEST_MAIN_SOURCES)
        file(RELATIVE_PATH rel_path "${ROOT_DIR}" "${main_src}")
        string(REPLACE "/" "_" exe_name "${rel_path}")
        string(REGEX REPLACE "\\.(cc|cpp)$" "" exe_name "${exe_name}")

        add_executable(${exe_name} "${main_src}")
        target_link_libraries(${exe_name} PRIVATE mylog)
        target_include_directories(${exe_name} PRIVATE "${ROOT_DIR}")

        if(NOT MSVC)
            target_compile_options(${exe_name} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()

    find_package(GTest CONFIG REQUIRED)
    enable_testing()

    add_executable(mylog_gtest_tests
        "${ROOT_DIR}/test/test_space.cpp"
    )
    target_link_libraries(mylog_gtest_tests PRIVATE mylog GTest::gtest GTest::gtest_main)
    target_include_directories(mylog_gtest_tests PRIVATE "${ROOT_DIR}")
    if(NOT MSVC)
        target_compile_options(mylog_gtest_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(GoogleTest)
    gtest_discover_tests(mylog_gtest_tests)
endif()

