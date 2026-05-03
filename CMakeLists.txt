cmake_minimum_required(VERSION 3.20)
project(myLog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MYLOG_BUILD_TESTS "Build test executables" ON)
option(MYLOG_USE_CRYPTOPP "Build Crypto++ from third_party sources" ON)
option(MYLOG_USE_PROTOBUF "Build Protobuf from third_party sources" ON)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}")

# 在 configure 第三方子目录时压低告警（不影响顶层 src/，configure 结束后恢复 FLAGS）
macro(mylog_add_subdirectory_quiet subdir)
    set(_mylog_save_cxx "${CMAKE_CXX_FLAGS}")
    set(_mylog_save_c "${CMAKE_C_FLAGS}")
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    endif()
    add_subdirectory(${subdir} ${ARGN})
    set(CMAKE_CXX_FLAGS "${_mylog_save_cxx}")
    set(CMAKE_C_FLAGS "${_mylog_save_c}")
endmacro()

mylog_add_subdirectory_quiet(third_party/fmt)
mylog_add_subdirectory_quiet(third_party/zlib)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Disable zstd programs" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Disable zstd tests" FORCE)
mylog_add_subdirectory_quiet(third_party/zstd/build/cmake)

if(MYLOG_USE_PROTOBUF)
    set(protobuf_BUILD_TESTS OFF CACHE BOOL "Disable protobuf tests" FORCE)
    set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "Disable protobuf examples" FORCE)
    set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "Disable protobuf conformance tests" FORCE)
    set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "Disable protoc binary build" FORCE)
    set(protobuf_BUILD_LIBPROTOC OFF CACHE BOOL "Disable libprotoc build" FORCE)
    set(protobuf_BUILD_LIBUPB OFF CACHE BOOL "Disable libupb build" FORCE)
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Build protobuf as static libraries" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "Disable protobuf install rules" FORCE)
    mylog_add_subdirectory_quiet(third_party/protobuf)
endif()

if(MYLOG_USE_CRYPTOPP)
    set(CRYPTOPP_ROOT "${ROOT_DIR}/third_party/cryptopp")
    if(NOT EXISTS "${CRYPTOPP_ROOT}/cryptlib.h")
        message(FATAL_ERROR "Crypto++ source not found. Expected ${CRYPTOPP_ROOT}/cryptlib.h")
    endif()

    file(GLOB CRYPTOPP_SOURCES
        CONFIGURE_DEPENDS
        "${CRYPTOPP_ROOT}/*.cpp"
    )
    list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX ".*/(adhoc|bench[0-9]*|datatest|dlltest|fipsalgt|fipsalgtvs|regtest|test|validat[0-9]*|cryptest)\\.cpp$")
    if(CRYPTOPP_SOURCES STREQUAL "")
        message(FATAL_ERROR "No Crypto++ library sources found under ${CRYPTOPP_ROOT}")
    endif()

    add_library(cryptopp STATIC ${CRYPTOPP_SOURCES})
    target_include_directories(cryptopp PRIVATE "${CRYPTOPP_ROOT}")
    target_compile_definitions(cryptopp PRIVATE CRYPTOPP_DISABLE_ASM=1)
    if(MSVC)
        target_compile_options(cryptopp PRIVATE /W0)
    else()
        target_compile_options(cryptopp PRIVATE -w)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
            target_compile_options(cryptopp PRIVATE -mssse3 -msse4.1)
        endif()
    endif()
endif()

file(GLOB_RECURSE ALL_SOURCES
    CONFIGURE_DEPENDS
    "${ROOT_DIR}/src/*.cc"
    "${ROOT_DIR}/src/*.cpp"
    "${ROOT_DIR}/test/*.cc"
    "${ROOT_DIR}/test/*.cpp"
)

list(FILTER ALL_SOURCES EXCLUDE REGEX [[/build/]])
list(FILTER ALL_SOURCES EXCLUDE REGEX [[/\\.git/]])
# 第三方库由各自 CMake 构建，不参与全局 GLOB
list(FILTER ALL_SOURCES EXCLUDE REGEX [[/third_party/]])

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
    message(FATAL_ERROR "No library sources found. Expected *.cc/*.cpp under src/.")
endif()

add_library(mylog STATIC ${LIB_SOURCES})
target_include_directories(mylog PUBLIC "${ROOT_DIR}/include")
target_link_libraries(mylog PUBLIC fmt::fmt)
target_link_libraries(mylog PRIVATE zlibstatic libzstd_static)
if(MYLOG_USE_PROTOBUF)
    # 与 effective_msg.proto 的 LITE_RUNTIME 及生成代码一致
    target_link_libraries(mylog PRIVATE protobuf::libprotobuf-lite)
    # utf8_range：parse_context.h 会 include "utf8_validity.h"
    target_include_directories(mylog PRIVATE
        "${ROOT_DIR}/third_party/protobuf/third_party/utf8_range"
    )
    # 生成头目录按 system include 处理，弱化 protoc 生成 .pb.h 被 -Wall 牵连的告警
    target_include_directories(mylog SYSTEM PRIVATE
        "${ROOT_DIR}/src/protobuf"
    )
    target_compile_definitions(mylog PUBLIC MYLOG_USE_PROTOBUF=1)

    # protoc 生成的 .pb.cc 常触发 -Wpedantic / conversion 等；仅对该文件关闭告警
    file(GLOB MYLOG_PROTOBUF_GEN_CC CONFIGURE_DEPENDS "${ROOT_DIR}/src/protobuf/*.pb.cc")
    if(MYLOG_PROTOBUF_GEN_CC)
        if(MSVC)
            set_source_files_properties(${MYLOG_PROTOBUF_GEN_CC} PROPERTIES COMPILE_OPTIONS "/W0")
        else()
            set_source_files_properties(${MYLOG_PROTOBUF_GEN_CC} PROPERTIES COMPILE_OPTIONS "-w")
        endif()
    endif()
endif()

if(MYLOG_USE_CRYPTOPP)
    target_link_libraries(mylog PRIVATE cryptopp)
    target_compile_definitions(mylog PUBLIC MYLOG_USE_CRYPTOPP=1)
endif()

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
        target_include_directories(${exe_name} PRIVATE "${ROOT_DIR}/include")

        if(NOT MSVC)
            target_compile_options(${exe_name} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endforeach()

    find_package(GTest CONFIG REQUIRED)
    enable_testing()

    add_executable(mylog_gtest_tests
        "${ROOT_DIR}/test/test_space.cpp"
        "${ROOT_DIR}/test/test_mmap.cpp"
    )
    target_link_libraries(mylog_gtest_tests PRIVATE mylog GTest::gtest GTest::gtest_main)
    target_include_directories(mylog_gtest_tests PRIVATE "${ROOT_DIR}/include")
    if(NOT MSVC)
        target_compile_options(mylog_gtest_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(GoogleTest)
    gtest_discover_tests(mylog_gtest_tests DISCOVERY_MODE PRE_TEST)
endif()

