#pragma once

#include "logging/logger_config.hpp"
#include "logging/using.hpp"

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