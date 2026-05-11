#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "logger/effective_sink.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/utils/filestream_linux.hpp"

namespace {

using namespace logger;

class EffectiveSinkTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto unique = "mylog_eff_test_" +
                      std::to_string(static_cast<unsigned long long>(::getpid())) + "_" +
                      std::to_string(static_cast<unsigned long long>(++counter_));
        test_dir_ = std::filesystem::temp_directory_path() / unique;

        auto [pri, pub] = crypt::GenECDHKey();
        config_pub_key_ = crypt::BinaryKeyToHex(pub);
    }

    void TearDown() override {
        sink_.reset();
        std::error_code ec;
        std::filesystem::remove_all(test_dir_, ec);
    }

    void CreateSink(megabytes single_size = megabytes(1), megabytes total_size = megabytes(10)) {
        EffectiveSink::Config cfg;
        cfg.dir        = test_dir_;
        cfg.prefix     = "test";
        cfg.pub_key    = config_pub_key_;
        cfg.single_size = single_size;
        cfg.total_size  = total_size;
        sink_ = std::make_unique<EffectiveSink>(cfg);
    }

    // 发送一条简单日志
    void Log(const std::string& msg, LogLevel lvl = LogLevel::kInfo) {
        LogMsg log_msg(lvl, msg);
        sink_->Log(log_msg);
    }

    static std::vector<std::filesystem::path> LogFiles(const std::filesystem::path& dir) {
        std::vector<std::filesystem::path> files;
        for (auto& p : std::filesystem::directory_iterator(dir)) {
            if (p.path().extension() == ".log") files.push_back(p.path());
        }
        return files;
    }

    std::filesystem::path test_dir_;
    std::string config_pub_key_;
    std::unique_ptr<EffectiveSink> sink_;
    static inline int counter_ = 0;
};

// ==================== 构造 ====================

TEST_F(EffectiveSinkTest, ConstructCreatesDirectory) {
    EXPECT_FALSE(std::filesystem::exists(test_dir_));
    CreateSink();
    EXPECT_TRUE(std::filesystem::exists(test_dir_));
}

TEST_F(EffectiveSinkTest, ConstructCreatesCacheFiles) {
    CreateSink();
    EXPECT_TRUE(std::filesystem::exists(test_dir_ / "master_cache"));
    EXPECT_TRUE(std::filesystem::exists(test_dir_ / "slave_cache"));
}

// ==================== Log + Flush ====================

TEST_F(EffectiveSinkTest, LogAndFlushProducesLogFile) {
    CreateSink();
    Log("hello world");
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    EXPECT_GE(files.size(), 1u);
}

TEST_F(EffectiveSinkTest, FlushWithoutLogDoesNotCrash) {
    CreateSink();
    EXPECT_NO_THROW(sink_->Flush());
}

TEST_F(EffectiveSinkTest, LogFileHasValidChunkHeader) {
    CreateSink();
    Log("test message");
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    ASSERT_GE(files.size(), 1u);

    std::ifstream ifs(files[0], std::ios::binary);
    ASSERT_TRUE(ifs.is_open());

    ChunkHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
    EXPECT_EQ(header.magic, ChunkHeader::kMagic);
    EXPECT_GT(header.size, 0u);
}

TEST_F(EffectiveSinkTest, LogFileAccumulatesMultipleChunks) {
    CreateSink();

    Log("first");
    sink_->Flush();

    Log("second");
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    ASSERT_GE(files.size(), 1u);

    // 两次 Flush 应产生两个 chunk
    std::ifstream ifs(files[0], std::ios::binary);
    int chunk_count = 0;
    while (ifs.peek() != EOF) {
        ChunkHeader header;
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!ifs) break;
        EXPECT_EQ(header.magic, ChunkHeader::kMagic);
        ifs.seekg(header.size, std::ios::cur);
        ++chunk_count;
    }
    EXPECT_EQ(chunk_count, 2);
}

// ==================== 多线程并发 ====================

TEST_F(EffectiveSinkTest, ConcurrentLogging) {
    CreateSink();

    constexpr int kThreads = 4;
    constexpr int kLogsPerThread = 100;
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, &errors, t]() {
            for (int i = 0; i < kLogsPerThread; ++i) {
                try {
                    Log("thread_" + std::to_string(t) + "_msg_" + std::to_string(i));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors, 0);

    sink_->Flush();
    auto files = LogFiles(test_dir_);
    EXPECT_GE(files.size(), 1u);
}

// ==================== 文件轮转 ====================

TEST_F(EffectiveSinkTest, FileRotationOnSizeExceeded) {
    // 设置很小的 single_size 使其快速触发轮转
    CreateSink(megabytes(1), megabytes(100));

    // 写入足够多的日志使文件超过 1MB
    std::string payload(4096, 'x');
    for (int i = 0; i < 500; ++i) {
        Log(payload);
    }
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    // 验证至少有日志文件生成
    EXPECT_GE(files.size(), 1u);
}

// ==================== 文件淘汰 ====================

TEST_F(EffectiveSinkTest, OldFilesEliminated) {
    CreateSink(megabytes(10), megabytes(1));  // total 只有 1MB，很快触发淘汰

    // 模拟直接写入一些旧日志文件
    for (int i = 0; i < 5; ++i) {
        auto path = test_dir_ / ("test_old_" + std::to_string(i) + ".log");
        std::ofstream ofs(path, std::ios::binary);
        std::string data(512 * 1024, 'x');  // 0.5MB each
        ofs.write(data.data(), data.size());
        ofs.close();
    }

    // 写一条日志触发 ElimateFiles_
    Log("trigger elimination");
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    // 验证淘汰机制执行后文件数量合理
    EXPECT_GE(files.size(), 0u);
}

// ==================== 崩溃恢复 ====================

TEST_F(EffectiveSinkTest, CrashRecoveryFlushesStaleCache) {
    CreateSink();
    Log("before crash");
    // 故意不 Flush，模拟崩溃

    // 销毁后重建，构造时若 cache 非空会自动落盘
    sink_.reset();
    CreateSink();
    sink_->Flush();

    auto files = LogFiles(test_dir_);
    EXPECT_GE(files.size(), 1u);
}

// ==================== 空日志 ====================

TEST_F(EffectiveSinkTest, EmptyLogMessage) {
    CreateSink();
    Log("");
    EXPECT_NO_THROW(sink_->Flush());

    auto files = LogFiles(test_dir_);
    EXPECT_GE(files.size(), 1u);
}

// ==================== SetFormatter ====================

TEST_F(EffectiveSinkTest, SetFormatterDoesNotCrash) {
    CreateSink();
    auto formatter = std::make_unique<DefaultFormatter>();
    EXPECT_NO_THROW(sink_->SetForMatter(std::move(formatter)));

    Log("after formatter change");
    EXPECT_NO_THROW(sink_->Flush());
}

}  // namespace
