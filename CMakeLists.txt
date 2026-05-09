cmake_minimum_required(VERSION 3.20)
project(myLog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(MYLOG_BUILD_TESTS "Build test executables" ON)
option(MYLOG_USE_CRYPTOPP "Build Crypto++ from third_party sources" ON)
option(MYLOG_USE_PROTOBUF "Build Protobuf from third_party sources" ON)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}")

add_subdirectory(third_party/fmt)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/zlib)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/zstd/build/cmake)

if(MYLOG_USE_PROTOBUF)
    if(NOT TARGET absl::strings)
        set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(ABSL_BUILD_TEST_HELPERS OFF CACHE BOOL "" FORCE)
        set(ABSL_USE_EXTERNAL_GOOGLETEST ON CACHE BOOL "" FORCE)
        set(ABSL_FIND_GOOGLETEST OFF CACHE BOOL "" FORCE)
        add_subdirectory(third_party/abseil-cpp)
    endif()

    set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_LIBPROTOC OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_LIBUPB OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/protobuf)
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
    target_include_directories(cryptopp PUBLIC "${CRYPTOPP_ROOT}")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
        target_compile_options(cryptopp PRIVATE -march=native)
    endif()
endif()

file(GLOB_RECURSE LIB_SOURCES
    CONFIGURE_DEPENDS
    "${ROOT_DIR}/src/logger/*.cc"
    "${ROOT_DIR}/src/logger/*.cpp"
)

list(FILTER LIB_SOURCES EXCLUDE REGEX [[/build/]])
list(FILTER LIB_SOURCES EXCLUDE REGEX [[/\\.git/]])

if(LIB_SOURCES STREQUAL "")
    message(FATAL_ERROR "No library sources found. Expected *.cc/*.cpp under src/logger/.")
endif()

add_library(mylog STATIC ${LIB_SOURCES})
target_include_directories(mylog PUBLIC
    "${ROOT_DIR}/include"
    "${ROOT_DIR}/third_party/zlib"
    "${ROOT_DIR}/third_party/zstd/lib"
)
target_link_libraries(mylog PUBLIC fmt::fmt)
target_link_libraries(mylog PRIVATE zlibstatic libzstd_static)
target_compile_definitions(mylog PUBLIC ENABLE_LOG=1)

if(MYLOG_USE_PROTOBUF)
    target_link_libraries(mylog PRIVATE protobuf::libprotobuf-lite)
    target_include_directories(mylog PRIVATE
        "${ROOT_DIR}/third_party/protobuf/third_party/utf8_range"
        "${ROOT_DIR}/src/logger/proto"
    )
    target_compile_definitions(mylog PUBLIC MYLOG_USE_PROTOBUF=1)
endif()

if(MYLOG_USE_CRYPTOPP)
    target_link_libraries(mylog PRIVATE cryptopp)
    target_compile_definitions(mylog PUBLIC MYLOG_USE_CRYPTOPP=1)
endif()

if(MYLOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
