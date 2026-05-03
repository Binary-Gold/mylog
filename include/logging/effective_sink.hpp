#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "logging/sink.hpp"
#include "utils/space.hpp"

namespace logger {
class EffectiveSink final : public LogSink {
public:
    struct Config {
        std::filesystem::path dir;        // 文件目录
        std::string prefix;               // 文件名前缀：{prefix}_{datetime}.log
        std::string pub_key;              // 公钥
        std::chrono::minutes interval{5}; // 淘汰间隔
        space::megabytes single_size{space::megabytes(4)};
        space::megabytes total_size{space::megabytes(100)};
    };

    explicit EffectiveSink(const Config& config);
    ~EffectiveSink() override;

    void Log(const LogMsg& msg) override;
    void SetForMatter(std::unique_ptr<ForMatter> formatter) override;
    void Flush() override;

private:
    void SwapCache_();

    bool NeedCacheToFile_();

    void WriteToCache_(const void* data, uint32_t size);

    void PrepareToFile_();

    void CacheToFile_();

    std::filesystem::path GetFilePath_();

    void ElimateFiles_();

    struct Imp;
    std::unique_ptr<Imp> imp_;
};
} // namespace logger
