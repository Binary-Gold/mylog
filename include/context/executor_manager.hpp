#pragma once

#include <memory>

#include "executor.hpp"
#include "using.hpp"

namespace context {
    class ExecutorManager {
    public:
        ExecutorManager();
        ~ExecutorManager();

        Executor* GetExecutor();
        TaskRunnerTag NewTaskRunner(TaskRunnerTag tag);
    private:
        struct Imp;
        std::unique_ptr<Imp> imp_;

    };
}
