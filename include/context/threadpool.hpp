#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <future>
#include <type_traits>

#include "context/using.hpp"

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
            EnqueueTask_(std::function<void()>([b = std::bind(std::forward<F>(f), std::forward<Args>(args)...)]() mutable { b(); }));
        }
        // 提交返回值任务
        template <typename F, typename... Args>
        // todo: 类型萃取可深入 
        auto RunRetTask(F&& f, Args&&... args) -> std::shared_ptr<std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>> {
            using ret_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
            auto task = std::make_shared<std::packaged_task<ret_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            std::future<ret_type> ret = task->get_future();

            if (EnqueueTask_(std::function<void()>([task]() { (*task)(); }))) {
                return std::make_shared<std::future<std::result_of_t<F(Args...)>>>(std::move(ret));
            } else {
                return nullptr;
            }
        }
    private:
        using ThreadPtr = std::shared_ptr<std::thread>;
        struct ThreadInfo {
            ThreadInfo() = default;
            ThreadInfo(ThreadPtr p) : ptr(p) {}
            ~ThreadInfo() { if (ptr && ptr->joinable()) { ptr->join(); } }

            ThreadPtr ptr{nullptr};
        };
        using ThreadInfoPtr = std::shared_ptr<ThreadInfo>;

        bool EnqueueTask_(Task task);
        void AddThread_();

        struct Imp;
        std::unique_ptr<Imp> imp_;


    };
}