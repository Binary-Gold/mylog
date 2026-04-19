#include <memory>
#include <future>

#include "context/executor.hpp"
#include "context/executor_context.hpp"
#include "context/executor_timer.hpp"

namespace context {
    struct Executor::Imp {
        std::unique_ptr<ExecutorContext> executor_context_;
        std::unique_ptr<ExecutorTimer> executor_timer_;
    };

    Executor::Executor() : imp_(std::make_unique<Imp>()) {
        imp_->executor_context_ = std::make_unique<ExecutorContext>();
        imp_->executor_timer_ = std::make_unique<ExecutorTimer>();
    }

    Executor::~Executor() = default;

    TaskRunnerTag Executor::AddTaskRunner(const TaskRunnerTag& tag) {
        return imp_->executor_context_->AddTaskRunner(tag);
    }
    void Executor::PostTask(const TaskRunnerTag& tag, Task task) {
        ExecutorContext::TaskRunner* task_runner = imp_->executor_context_->GetTaskRunner_(tag);
        task_runner->RunTask(std::move(task));
    }

    void Executor::PostDelayedTask_(Task task, const std::chrono::microseconds& delta) {
        imp_->executor_timer_->Start();
        imp_->executor_timer_->PostDelayedTask(std::move(task), delta);
    }

    RepeatedTaskId Executor::PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num) {
        imp_->executor_timer_->Start();
        return imp_->executor_timer_->PostRepeatedTask(std::move(task), delta, repeat_num);
    }

    void Executor::CancelRepeatedTask(RepeatedTaskId task_id) {
        imp_->executor_timer_->CancelRepeatedTask(task_id);
    }
    ExecutorContext::TaskRunner* Executor::GetTaskRunner_(const TaskRunnerTag& tag) {
        return imp_->executor_context_->GetTaskRunner_(tag);
    }
}
