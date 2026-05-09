#include <cstddef>
#include <iostream>
#include <string>

#include "logger/logger_config.hpp"
#include "logger/logger_handle.hpp"
#include "logger/sink.hpp"

int main() {
    auto s1 = std::make_shared<logger::ConsoleSink>();
    auto s2 = std::make_shared<logger::ConsoleSink>();


    logger::LogHandle handle{s1, s2};
    logger::SourceLocation sourcelocation(__FILE__, __LINE__, __FUNCTION__);

    handle.Log(logger::LogLevel::kInfo, sourcelocation, "hello word\n");

    return 0;
}
