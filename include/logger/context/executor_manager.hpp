#pragma once

#include <memory>

#include "logger/context/executor.hpp"
#include "logger/context/using.hpp"

namespace logger::ctx {
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
