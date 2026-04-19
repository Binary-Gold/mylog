#include "context/executor_manager.hpp"

namespace context {
    struct ExecutorManager::Imp {
        std::unique_ptr<Executor> executor_;
    };

    ExecutorManager::ExecutorManager() : imp_(std::make_unique<Imp>()) {
        imp_->executor_ = std::make_unique<Executor>();
    }
    ExecutorManager::~ExecutorManager() = default;

    Executor* ExecutorManager::GetExecutor() {
        return imp_->executor_.get();
    }

    TaskRunnerTag ExecutorManager::NewTaskRunner(TaskRunnerTag tag) {
        return imp_->executor_->AddTaskRunner(tag);
    }
}
