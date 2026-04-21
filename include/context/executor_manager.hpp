#pragma once

#include <memory>

#include "context/executor.hpp"
#include "context/using.hpp"

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
