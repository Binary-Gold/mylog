#pragma once

#include <memory>

#include "context/executor_timer.hpp"
#include "context/threadpool.hpp"
#include "context/using.hpp"

namespace context {
    class ExecutorContext {
    public:
        ExecutorContext();
        ~ExecutorContext();

        ExecutorContext(const ExecutorContext&) = delete;
        ExecutorContext& operator=(const ExecutorContext&) = delete;

        TaskRunnerTag AddTaskRunner(const TaskRunnerTag& tag);
    private:
        using TaskRunner = ThreadPool;
        using TaskRunnerPtr = std::unique_ptr<TaskRunner>;
        friend class Executor;

        TaskRunner* GetTaskRunner_(const TaskRunnerTag& tag);
        TaskRunnerTag GetNextRunnerTag_();

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}
