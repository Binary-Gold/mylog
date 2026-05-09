#ifndef LOGGER_CONFIG_HPP
#define LOGGER_CONFIG_HPP

#include <cstdint>
#include <string_view>


namespace logger {
    
using MemoryBuf = std::string;

struct SourceLocation {
    constexpr SourceLocation() = default;
    SourceLocation(std::string_view file_name_in, int32_t line_in, std::string_view func_name_in)
        : file_name(file_name_in), line(line_in), func_name(func_name_in) {
        if (!file_name.empty()) {
            size_t pos = file_name.rfind('/');
            if (pos != std::string_view::npos) {
                file_name = file_name.substr(pos + 1);
            } else {
                pos = file_name.rfind('\\');
                if (pos != std::string_view::npos) {
                    file_name = file_name.substr(pos + 1);
                }
            }
        }
    }

    std::string_view file_name;
    int32_t line{0};
    std::string_view func_name;
};

#define LOGGER_LEVEL_TRACE 0
#define LOGGER_LEVEL_DEBUG 1
#define LOGGER_LEVEL_INFO 2
#define LOGGER_LEVEL_WARN 3
#define LOGGER_LEVEL_ERROR 4
#define LOGGER_LEVEL_CRITICAL 5
#define LOGGER_LEVEL_OFF 6

enum class LogLevel {
  kTrace = LOGGER_LEVEL_TRACE,
  kDebug = LOGGER_LEVEL_DEBUG,
  kInfo = LOGGER_LEVEL_INFO,
  kWarn = LOGGER_LEVEL_WARN,
  kError = LOGGER_LEVEL_ERROR,
  kFatal = LOGGER_LEVEL_CRITICAL,
  kOff = LOGGER_LEVEL_OFF
};

struct LogMsg {
    LogMsg(SourceLocation loc, LogLevel lvl, std::string_view msg) : location(std::move(loc)), level(lvl), message(std::move(msg)) {}

    LogMsg(LogLevel lvl, std::string_view msg) : LogMsg(SourceLocation{}, lvl, msg) {}

    LogMsg(const LogMsg& other) = default;
    LogMsg& operator=(const LogMsg& other) = default;

    SourceLocation location;
    LogLevel level;
    std::string_view message;
};
}  // namespace logger

#endif  // LOGGER_CONFIG_HPP
