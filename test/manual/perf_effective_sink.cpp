#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>

#include "logger/effective_sink.hpp"
#include "logger/crypt/crypt.hpp"

using namespace logger;
using Clock = std::chrono::high_resolution_clock;

struct Result {
    std::string label;
    int ops;
    double elapsed_s;
    double ops_per_sec;
    double avg_us;
    double p50_us;
    double p99_us;
    double max_us;
};

void Sep() { std::cout << std::string(114, '-') << '\n'; }

void Header() {
    std::cout << std::left  << std::setw(30) << "Test"
              << std::right << std::setw(8)  << "ops"
              << std::setw(10) << "elapsed"
              << std::setw(11) << "ops/s"
              << std::setw(10) << "avg"
              << std::setw(10) << "p50"
              << std::setw(10) << "p99"
              << std::setw(10) << "max"
              << '\n';
    Sep();
}

void Print(const Result& r) {
    std::cout << std::left  << std::setw(30) << r.label
              << std::right << std::setw(8)  << r.ops
              << std::fixed   << std::setprecision(3)
              << std::setw(7) << r.elapsed_s << "s"
              << std::setw(10) << static_cast<int>(r.ops_per_sec)
              << std::setw(9)  << static_cast<int>(r.avg_us) << "us"
              << std::setw(9)  << static_cast<int>(r.p50_us) << "us"
              << std::setw(9)  << static_cast<int>(r.p99_us) << "us"
              << std::setw(9)  << static_cast<int>(r.max_us) << "us"
              << '\n';
}

EffectiveSink::Config MakeConfig(const std::filesystem::path& dir, const std::string& prefix) {
    auto [pri, pub] = crypt::GenECDHKey();
    EffectiveSink::Config cfg;
    cfg.dir         = dir;
    cfg.prefix      = prefix;
    cfg.pub_key     = crypt::BinaryKeyToHex(pub);
    cfg.single_size = megabytes(64);
    cfg.total_size  = megabytes(256);
    return cfg;
}

Result Measure(EffectiveSink& sink, const std::string& label,
               int ops, const std::string& msg) {
    std::vector<double> lat(ops);

    // warmup
    for (int i = 0; i < std::min(200, ops / 5); ++i) {
        LogMsg m(LogLevel::kDebug, msg);
        sink.Log(m);
    }

    auto t0 = Clock::now();
    for (int i = 0; i < ops; ++i) {
        LogMsg m(LogLevel::kDebug, msg);
        auto t1 = Clock::now();
        sink.Log(m);
        auto t2 = Clock::now();
        lat[i] = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()) / 1000.0;
    }
    auto t3 = Clock::now();

    double el = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(t3 - t0).count()) / 1'000'000.0;

    std::sort(lat.begin(), lat.end());

    Result r;
    r.label      = label;
    r.ops        = ops;
    r.elapsed_s  = el;
    r.ops_per_sec = el > 0 ? ops / el : 0;
    r.avg_us     = std::accumulate(lat.begin(), lat.end(), 0.0) / ops;
    r.p50_us     = lat[ops / 2];
    r.p99_us     = lat[ops * 99 / 100];
    r.max_us     = lat.back();
    return r;
}

Result MeasureConcurrent(EffectiveSink& sink, const std::string& label,
                          int total, int n_threads, const std::string& msg) {
    int per_thread = total / n_threads;
    std::vector<std::vector<double>> all(n_threads);
    std::atomic<int> done{0};

    // warmup: 每个线程跑一点
    {
        std::vector<std::thread> warm;
        for (int t = 0; t < n_threads; ++t)
            warm.emplace_back([&, t]() {
                for (int i = 0; i < 50; ++i) {
                    LogMsg m(LogLevel::kInfo, msg);
                    sink.Log(m);
                }
            });
        for (auto& th : warm) th.join();
    }

    auto t0 = Clock::now();
    std::vector<std::thread> threads;
    for (int t = 0; t < n_threads; ++t) {
        threads.emplace_back([&, t]() {
            auto& lat = all[t];
            lat.reserve(per_thread);
            for (int i = 0; i < per_thread; ++i) {
                LogMsg m(LogLevel::kInfo, msg);
                auto t1 = Clock::now();
                sink.Log(m);
                auto t2 = Clock::now();
                auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
                lat.push_back(static_cast<double>(ns) / 1000.0);
                ++done;
            }
        });
    }
    for (auto& th : threads) th.join();
    auto t3 = Clock::now();

    int actual = done.load();
    double el = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(t3 - t0).count()) / 1'000'000.0;

    std::vector<double> merged;
    for (auto& v : all) merged.insert(merged.end(), v.begin(), v.end());
    std::sort(merged.begin(), merged.end());

    int n = static_cast<int>(merged.size());
    Result r;
    r.label      = label;
    r.ops        = actual;
    r.elapsed_s  = el;
    r.ops_per_sec = el > 0 ? actual / el : 0;
    r.avg_us     = n > 0 ? std::accumulate(merged.begin(), merged.end(), 0.0) / n : 0;
    r.p50_us     = n > 0 ? merged[n / 2] : 0;
    r.p99_us     = n > 0 ? merged[n * 99 / 100] : 0;
    r.max_us     = n > 0 ? merged.back() : 0;
    return r;
}

void CleanDir(const std::filesystem::path& dir) {
    std::error_code ec;
    for (auto& p : std::filesystem::directory_iterator(dir, ec)) {
        std::filesystem::remove_all(p, ec);
    }
}

int main() {
    auto base = std::filesystem::temp_directory_path() / "mylog_perf";
    std::filesystem::remove_all(base);

    // 消息模板
    auto small  = std::string("[INFO] short message");
    auto medium = std::string(
        "[INFO] GET /api/v1/users/12345/profile status=200 dur=45ms "
        "agent=chrome/120 tid=abc123def456 uid=alice rsize=4096");
    auto large  = std::string(
        "[ERROR] POST /api/v1/data/batch status=500 dur=3200ms "
        "err=\"connection timeout after 3000ms to database host=10.0.1.50:5432 "
        "query=INSERT INTO events VALUES (...)\" "
        "tid=xyz789 retry=3 stack=database.go:245:func WriteBatch() "
        "caller=api_handler.go:102:func HandleBatch() uid=bob");

    // ==================== 单线程吞吐 ====================
    std::cout << "\n  EffectiveSink 性能测试 (EffectiveFormatter + Zstd + AES-128-CBC)\n\n";
    std::cout << "  === 单线程吞吐 ===\n\n";
    Header();

    {
        auto dir = base / "st_small";
        auto cfg = MakeConfig(dir, "small");
        EffectiveSink sink(cfg);
        auto r = Measure(sink, "small  (~50B)",  3000, small);
        Print(r);
        sink.Flush();
        CleanDir(dir);
    }

    {
        auto dir = base / "st_medium";
        auto cfg = MakeConfig(dir, "medium");
        EffectiveSink sink(cfg);
        auto r = Measure(sink, "medium (~200B)", 3000, medium);
        Print(r);
        sink.Flush();
        CleanDir(dir);
    }

    {
        auto dir = base / "st_large";
        auto cfg = MakeConfig(dir, "large");
        EffectiveSink sink(cfg);
        auto r = Measure(sink, "large  (~380B)", 2000, large);
        Print(r);
        sink.Flush();
        CleanDir(dir);
    }

    // ==================== 多线程并发 ====================
    std::cout << "\n  === 多线程并发 (medium ~200B, 8000 ops) ===\n\n";
    Header();

    for (int threads : {1, 2, 4, 8}) {
        auto dir = base / ("mt_" + std::to_string(threads));
        auto cfg = MakeConfig(dir, "mt");
        EffectiveSink sink(cfg);
        auto r = MeasureConcurrent(sink,
                                   std::to_string(threads) + " threads", 8000, threads, medium);
        Print(r);
        sink.Flush();
        CleanDir(dir);
    }

    // ==================== Flush 延迟 ====================
    std::cout << "\n  === Flush 延迟 ===\n\n";
    {
        auto dir = base / "flush";
        auto cfg = MakeConfig(dir, "flush");
        EffectiveSink sink(cfg);

        for (int n : {100, 1000, 5000}) {
            for (int i = 0; i < n; ++i) {
                LogMsg m(LogLevel::kInfo, medium);
                sink.Log(m);
            }
            auto t0 = Clock::now();
            sink.Flush();
            auto t1 = Clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
            std::cout << "  " << std::setw(5) << n << " 条日志 Flush: "
                      << std::setw(6) << us << " us ("
                      << std::fixed << std::setprecision(2)
                      << static_cast<double>(us) / 1000.0 << " ms)\n";
            CleanDir(dir);
        }
    }

    // ==================== 落盘压缩率 ====================
    std::cout << "\n  === 落盘体积 ===\n\n";
    {
        auto dir = base / "disk";
        auto cfg = MakeConfig(dir, "disk");
        EffectiveSink sink(cfg);

        // 写入 1000 条 medium 日志
        size_t raw_bytes = 0;
        for (int i = 0; i < 1000; ++i) {
            raw_bytes += medium.size();
            LogMsg m(LogLevel::kInfo, medium);
            sink.Log(m);
        }
        sink.Flush();

        uint64_t file_bytes = 0;
        int file_count = 0;
        for (auto& p : std::filesystem::directory_iterator(dir)) {
            if (p.path().extension() == ".log") {
                file_bytes += std::filesystem::file_size(p);
                ++file_count;
            }
        }

        std::cout << "  原始日志体积:   " << raw_bytes << " bytes ("
                  << std::fixed << std::setprecision(1)
                  << static_cast<double>(raw_bytes) / 1024.0 << " KB)\n";
        std::cout << "  落盘文件体积:   " << file_bytes << " bytes ("
                  << static_cast<double>(file_bytes) / 1024.0 << " KB)\n";
        std::cout << "  压缩率:         "
                  << std::setprecision(1)
                  << static_cast<double>(raw_bytes) / file_bytes << "x\n";
        std::cout << "  日志文件数:     " << file_count << "\n";
    }

    Sep();
    std::cout << "\n  Pipeline: LogMsg → EffectiveFormatter(protobuf) → ZstdCompress → AES-128-CBC → mmap → File\n";
    std::cout << "  编译器: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
    std::cout << "  C++ 标准: C++" << __cplusplus / 100 << "\n";

    std::filesystem::remove_all(base);
    return 0;
}
