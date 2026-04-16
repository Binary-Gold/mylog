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
#include <future>
#include <type_traits>

namespace context {
    class ThreadPool {
    public:
        explicit ThreadPool(uint32_t thread_count);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        bool Start();
        void Stop();

        // 提交普通任务
        template <typename F, typename... Args>
        void RunTask(F&& f, Args&&... args) {
            EnqueueTask_(std::function<void()>(
                [b = std::bind(std::forward<F>(f), std::forward<Args>(args)...)]() mutable { b(); }));
        }
        // 提交返回值任务
        template <typename F, typename... Args>
        auto RunRetTask(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            using ret_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
            auto task = std::make_shared<std::packaged_task<ret_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::future<ret_type> res = task->get_future();
            EnqueueTask_(std::function<void()>([task]() { (*task)(); }));
            return res;
        }
    private:
        using Task = std::function<void()>;
        // todo: 这里需要优化，使用裸指针可能更好
        using ThreadPtr = std::shared_ptr<std::thread>;
        struct ThreadInfo {
            ThreadInfo() = default;
            ThreadInfo(ThreadPtr p) : ptr(p) {};
            ~ThreadInfo();

            ThreadPtr ptr{nullptr};
        };
        using ThreadInfoPtr = std::shared_ptr<ThreadInfo>;

        void EnqueueTask_(Task task);
        void AddThread_();

        struct Imp;
        std::unique_ptr<Imp> imp_;


    };
}