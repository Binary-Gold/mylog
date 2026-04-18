#include <memory>
#include <future>

#include "context/excutor.hpp"
#include "context/excutor_context.hpp"
#include "context/excutor_timer.hpp"

namespace context {
    struct Excutor::Imp {
        std::unique_ptr<ExcutorContext> excutor_context_;
        std::unique_ptr<ExcutorTimer> excutor_timer_;
    };
    
    TaskRunnerTag Excutor::AddTaskRunner(const TaskRunnerTag& tag) {
        return imp_->excutor_context_->AddTaskRunner(tag);
    }
    void Excutor::PostTask(const TaskRunnerTag& tag, Task task) {
        ExcutorContext::TaskRunner* task_runner = imp_->excutor_context_->GetTaskRunner_(tag);
        task_runner->RunTask(std::move(task));
    }

    void Excutor::PostDelayedTask_(Task task, const std::chrono::microseconds& delta) {
        imp_->excutor_timer_->Start();
        imp_->excutor_timer_->PostDelayedTask(std::move(task), delta);
    }

    RepeatedTaskId Excutor::PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num) {
        imp_->excutor_timer_->Start();
        return imp_->excutor_timer_->PostRepeatedTask(std::move(task), delta, repeat_num);
    }

    void Excutor::CanCelRepeatedTask(RepeatedTaskId task_id) {
        imp_->excutor_timer_->CancleRepeatedTask(task_id);
    }
    ExcutorContext::TaskRunner* Excutor::GetTaskRunner_(const TaskRunnerTag& tag) {
        return imp_->excutor_context_->GetTaskRunner_(tag);
    }
}