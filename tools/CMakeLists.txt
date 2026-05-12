set(TOOLS_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/src/logger/proto"
    "${CMAKE_SOURCE_DIR}/third_party/protobuf/src"
    "${CMAKE_SOURCE_DIR}/third_party/abseil-cpp"
    "${CMAKE_SOURCE_DIR}/third_party/protobuf/third_party/utf8_range"
)

add_executable(log_parser log_parser.cpp)
target_link_libraries(log_parser PRIVATE mylog)
target_include_directories(log_parser PRIVATE ${TOOLS_INCLUDE_DIRS})

add_executable(test_roundtrip test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE mylog)
target_include_directories(test_roundtrip PRIVATE ${TOOLS_INCLUDE_DIRS})

include(GNUInstallDirs)
install(TARGETS log_parser test_roundtrip
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
