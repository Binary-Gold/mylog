#pragma once

#include "logger/formatter.hpp"

namespace logger {
class EffectiveFormatter : public ForMatter {
public:
    EffectiveFormatter() = default;
    ~EffectiveFormatter() override = default;

    void Fromat(const LogMsg& msg, MemoryBuf* dest) override;
};
} // namespace logger
