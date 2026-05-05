cmake_minimum_required(VERSION 3.20)

# ---- 判断是独立构建还是被顶层 add_subdirectory 包含 ----
if(NOT TARGET mylog)
    # ========== 独立构建模式 ==========
    project(mylog_test LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(MYLOG_BLD "${CMAKE_SOURCE_DIR}/../build")
    set(MYLOG_INC "${CMAKE_SOURCE_DIR}/../include")

    # -- mylog 库 --
    find_library(MYLOG_LIB mylog PATHS "${MYLOG_BLD}" NO_DEFAULT_PATH REQUIRED)
    add_library(mylog STATIC IMPORTED)
    set_target_properties(mylog PROPERTIES IMPORTED_LOCATION "${MYLOG_LIB}")
    target_include_directories(mylog INTERFACE "${MYLOG_INC}")

    # -- 传递依赖（静态库需要）--
    find_library(FMT_LIB fmt  PATHS "${MYLOG_BLD}/third_party/fmt"  NO_DEFAULT_PATH REQUIRED)
    find_library(Z_LIB   z    PATHS "${MYLOG_BLD}/third_party/zlib" NO_DEFAULT_PATH REQUIRED)
    find_package(Threads REQUIRED)
endif()

# ---- gtest（顶层和独立都需要，且仅在未找到时查找）----
if(NOT TARGET GTest::gtest)
    find_package(GTest CONFIG REQUIRED)
endif()

# ========== 可执行目标 ==========

# -- test_main（手动测试入口）--
add_executable(test_main main.cpp)
target_link_libraries(test_main PRIVATE mylog)

# -- mylog_gtest_tests（gtest 单元测试）--
add_executable(mylog_gtest_tests
    test_space.cpp
    test_mmap.cpp
)
target_link_libraries(mylog_gtest_tests PRIVATE mylog GTest::gtest GTest::gtest_main)

# 独立构建时补齐传递依赖
if(NOT TARGET fmt::fmt)
    target_link_libraries(test_main        PRIVATE "${FMT_LIB}" "${Z_LIB}" Threads::Threads)
    target_link_libraries(mylog_gtest_tests PRIVATE "${FMT_LIB}" "${Z_LIB}" Threads::Threads)
endif()

# -- 注册测试 --
enable_testing()
add_test(NAME mylog_gtest COMMAND mylog_gtest_tests)
