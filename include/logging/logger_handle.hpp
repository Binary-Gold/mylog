#ifndef LOGGER_HANDLE_HPP
#define LOGGER_HANDLE_HPP

#include <string_view>

#include "logging/logger_config.hpp"

namespace logger {
class LogHandle {
public:
    LogHandle() = default;
    ~LogHandle() = default;

    void Log(LogLevel lvl, SourceLocation loc, std::string_view msg);

private:
    void Log_(const LogMsg& msg);
};
}  // namespace logger

#endif  // LOGGER_HANDLE_HPP
