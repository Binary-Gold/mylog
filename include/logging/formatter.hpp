#pragma once

#include "using.hpp"
#include "logger_config.hpp"

namespace logger {
    class ForMatter {
    public:
        virtual ~ForMatter() = default;

        virtual void Fromat(const LogMsg& msg, MemoryBuf* dest) = 0;
    };

    class DefaultFormatter : public ForMatter {
    public:
        void Fromat(const LogMsg& msg, MemoryBuf* dest) override;
    };
}