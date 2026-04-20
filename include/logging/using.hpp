#pragma once

#include <memory>
#include <initializer_list>
#include <string>

#define LOGGER_LEVEL_TRACE 0
#define LOGGER_LEVEL_DEBUG 1
#define LOGGER_LEVEL_INFO 2
#define LOGGER_LEVEL_WARN 3
#define LOGGER_LEVEL_ERROR 4
#define LOGGER_LEVEL_CRITICAL 5
#define LOGGER_LEVEL_OFF 6

namespace logger {
    class LogSink;
    using LogSinkPtr = std::shared_ptr<LogSink>;
    using LogSinkPtrList = std::initializer_list<LogSinkPtr>;
    using MemoryBuf = std::string;
}