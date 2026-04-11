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

        // 提交普通任务：模板只打包 callable
        template <typename F, typename... Args>
        void RunTask(F&& f, Args&&... args) {
            EnqueueTask_(std::function<void()>(
                [b = std::bind(std::forward<F>(f), std::forward<Args>(args)...)]() mutable { b(); }));
        }

    private:
        using Task = std::function<void()>;
        using ThreadPtr = std::shared_ptr<std::thread>;
        struct ThreadInfo {
            ThreadInfo() = default;
            ThreadInfo(ThreadPtr p) : ptr(p) {};
            ~ThreadInfo();

            ThreadPtr ptr{nullptr};
        };
        using ThreadInfoPtr = std::shared_ptr<ThreadInfo>;

        void EnqueueTask_(Task task);
        void AddThread();

        struct Imp;
        std::unique_ptr<Imp> imp_;


    };
}