#include "context/context.hpp"
#include "context/executor_manager.hpp"

namespace context {
    struct Context::Imp {
        std::unique_ptr<ExecutorManager> executor_manager_ = std::make_unique<ExecutorManager>();
    };

    Context::Context() : imp_(std::make_unique<Imp>()) {}
    Context::~Context() = default;

    Executor* Context::GetExecutor() {
        return imp_->executor_manager_->GetExecutor();
    }

    TaskRunnerTag Context::NewTaskRunner(TaskRunnerTag tag) {
        return imp_->executor_manager_->NewTaskRunner(tag);
    }
}
