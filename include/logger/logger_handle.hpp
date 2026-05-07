#pragma once

#include <string_view>
#include <memory>
#include <initializer_list>

#include "logger/logger_config.hpp"
#include "logger/sink.hpp"

namespace logger {
class LogHandle {
    using LogSinkPtr = std::shared_ptr<LogSink>;
    using LogSinkPtrList = std::initializer_list<LogSinkPtr>;
public:
    explicit LogHandle(LogSinkPtrList sinks);
    explicit LogHandle(LogSinkPtr sink);
    template<typename It>
    LogHandle(It begin, It end) : LogHandle(LogSinkPtrList(begin, end)) {}
    ~LogHandle();

    LogHandle(const LogHandle&) = delete;
    LogHandle& operator=(const LogHandle&) = delete;

    void SetLevel(LogLevel level);
    LogLevel GetLevel() const;

    void Log(LogLevel lvl, SourceLocation loc, std::string_view msg);
protected:
    void Log_(const LogMsg& log_msg);
    bool ShouldLog_(LogLevel level) const noexcept;
private:
    struct Imp;
    std::unique_ptr<Imp> imp_;
};
} 
