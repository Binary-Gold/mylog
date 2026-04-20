#pragma once
#include <chrono>
#include <future>
#include <type_traits>

#include "using.hpp"
#include "executor_context.hpp"

namespace context{
    class Executor
    {
    public:
        Executor();
        ~Executor();

        Executor(const Executor&) = delete;
        Executor& operator=(const Executor&) = delete;

        TaskRunnerTag AddTaskRunner(const TaskRunnerTag& tag);
        void PostTask(const TaskRunnerTag& tag, Task task);

        template<typename R, typename P>
        void PostDelayedTask(const TaskRunnerTag& tag, Task task, const std::chrono::duration<R, P>& delta) {
            Task func = std::bind(&Executor::PostTask, this, tag, std::move(task));
            PostDelayedTask_(func, std::chrono::duration_cast<std::chrono::microseconds>(delta));
        }

        template<typename R, typename P>
        RepeatedTaskId PostRepeatedTask(const TaskRunnerTag& tag, Task task, const std::chrono::duration<R, P>& delta, uint64_t repeat_num) {
            Task func = std::bind(&Executor::PostTask, this, tag, std::move(task));
            return PostRepeatedTask_(func, std::chrono::duration_cast<std::chrono::microseconds>(delta), repeat_num);
        }
        void CancelRepeatedTask(RepeatedTaskId task_id);

        template<typename F, typename... Args>
        auto PostTaskAndGetResult(const TaskRunnerTag& tag, F&& f, Args&&... args) -> std::shared_ptr<std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>> {
            ExecutorContext::TaskRunner* task_runner = GetTaskRunner_(tag);
            if (task_runner == nullptr) {
            // todo throw?
            return nullptr;
        }
            auto ret = task_runner->RunRetTask(std::forward<F>(f), std::forward<Args>(args)...);
            return std::make_shared<std::future<std::result_of_t<F(Args...)>>>(std::move(ret));
        }
    private:
        void PostDelayedTask_(Task task, const std::chrono::microseconds& delta);
        RepeatedTaskId PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num);
        ExecutorContext::TaskRunner* GetTaskRunner_(const TaskRunnerTag& tag);

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };

}
