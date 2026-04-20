#pragma once

#include <memory>

#include "logger_config.hpp"
#include "formatter.hpp"

namespace logger {
    class LogSink {
    public:
        virtual ~LogSink() = default;
        virtual void Log(const LogMsg& msg) = 0;
        virtual void SetForMatter(std::unique_ptr<ForMatter> formatter) = 0;
        virtual void Flush() {}
    };

    class ConsoleSink : public LogSink {
    public:
        ConsoleSink();
        ~ConsoleSink() override;

        void Log(const LogMsg& msg) override;
        void SetForMatter(std::unique_ptr<ForMatter> formatter) override;
    private:
        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
    
}