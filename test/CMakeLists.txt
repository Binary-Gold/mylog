cmake_minimum_required(VERSION 3.20)

# ---- 独立构建 / 顶层包含 兼容 ----
if(NOT TARGET mylog)
    project(mylog_test LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    endif()

    set(MYLOG_BLD "${CMAKE_SOURCE_DIR}/../build")
    set(MYLOG_INC "${CMAKE_SOURCE_DIR}/../include")

    set(MYLOG_THIRD "${CMAKE_SOURCE_DIR}/../third_party")

    find_library(MYLOG_LIB mylog PATHS "${MYLOG_BLD}" NO_DEFAULT_PATH REQUIRED)
    add_library(mylog STATIC IMPORTED)
    set_target_properties(mylog PROPERTIES IMPORTED_LOCATION "${MYLOG_LIB}")
    target_include_directories(mylog INTERFACE
        "${MYLOG_INC}"
        "${MYLOG_THIRD}/fmt/include"
    )
    target_compile_definitions(mylog INTERFACE ENABLE_LOG=1)

    find_library(FMT_LIB NAMES fmt fmtd PATHS "${MYLOG_BLD}/third_party/fmt" NO_DEFAULT_PATH REQUIRED)
    find_library(Z_LIB   z    PATHS "${MYLOG_BLD}/third_party/zlib" NO_DEFAULT_PATH REQUIRED)
    find_package(Threads REQUIRED)
endif()

if(NOT TARGET GTest::gtest)
    find_package(GTest CONFIG REQUIRED)
endif()

enable_testing()

# ---- 辅助宏：添加 gtest 单元测试 ----
# add_unit_test(name) → 从 unit/<name>.cpp 创建可执行文件，链接 gtest_main
macro(add_unit_test name)
    add_executable("ut_${name}" "unit/${name}.cpp")
    target_link_libraries("ut_${name}" PRIVATE mylog GTest::gtest GTest::gtest_main)
    if(NOT TARGET fmt::fmt)
        target_link_libraries("ut_${name}" PRIVATE "${FMT_LIB}" "${Z_LIB}" Threads::Threads)
    endif()
    add_test(NAME "unit/${name}" COMMAND "ut_${name}")
endmacro()

# ---- 辅助宏：添加独立测试（自带 main） ----
# add_manual_test(name) → 从 manual/<name>.cpp 创建可执行文件
macro(add_manual_test name)
    add_executable("manual_${name}" "manual/${name}.cpp")
    target_link_libraries("manual_${name}" PRIVATE mylog)
    if(NOT TARGET fmt::fmt)
        target_link_libraries("manual_${name}" PRIVATE "${FMT_LIB}" "${Z_LIB}" Threads::Threads)
    endif()
    add_test(NAME "manual/${name}" COMMAND "manual_${name}")
endmacro()

# ---- 自动发现并注册 ----
file(GLOB UNIT_TESTS   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")
file(GLOB MANUAL_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/manual/*.cpp")

foreach(src ${UNIT_TESTS})
    get_filename_component(name ${src} NAME_WE)
    add_unit_test(${name})
endforeach()

foreach(src ${MANUAL_TESTS})
    get_filename_component(name ${src} NAME_WE)
    add_manual_test(${name})
endforeach()
