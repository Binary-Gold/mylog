#include <atomic>
#include <vector>

#include "logging/logger_handle.hpp"
#include "logging/sink.hpp"

namespace logger {
    struct LogHandle::Imp {
        std::atomic<LogLevel> level_;
        std::vector<LogSinkPtr> sinks_;
    };

    LogHandle::LogHandle(LogSinkPtrList sinks) : imp_(std::make_unique<Imp>()) {
        for (auto& sink : sinks) {
            imp_->sinks_.push_back(std::move(sink));
        }
    }
    LogHandle::LogHandle(LogSinkPtr sink) : imp_(std::make_unique<Imp>()) {
        imp_->sinks_.push_back(std::move(sink));
    }

    void LogHandle::SetLevel(LogLevel level) {
        imp_->level_ = level;
    }

    LogLevel LogHandle::GetLevel() const {
        return imp_->level_;
    }

    void LogHandle::Log(LogLevel lvl, SourceLocation loc, std::string_view message) {
        if (!ShouldLog_(lvl)) {
            return;
        }

        LogMsg msg(loc, lvl, message);
        Log_(msg);
    }

    bool LogHandle::ShouldLog_(LogLevel level) const noexcept {
        return level >= imp_->level_ && !imp_->sinks_.empty();
    }

    void LogHandle::Log_(const LogMsg& log_msg) {
        for (auto& sink : imp_->sinks_) {
            sink->Log(log_msg);
        }
    }
}