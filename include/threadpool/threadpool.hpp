#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

namespace threadpool {
    class ThreadPool {
    public:
        explicit ThreadPool(uint32_t thread_count);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        bool Start();
        void Stop();


    private:
        void AddThread();

        struct Imp;
        std::unique_ptr<Imp> imp_;

        using ThreadPtr = std::shared_ptr<std::thread>;
        struct ThreadInfo {
            ThreadInfo() = default;
            ~ThreadInfo();

            ThreadPtr ptr{nullptr};
        };
        using ThreadInfoPtr = std::shared_ptr<ThreadInfo>;
        using Task = std::function<void()>;
    };
}