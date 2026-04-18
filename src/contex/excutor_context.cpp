#include <unordered_map>

#include "context/excutor_context.hpp"

namespace context{
    struct ExcutorContext::Imp {
        std::unordered_map<TaskRunnerTag, TaskRunnerPtr> task_runner_dict_;
        std::mutex mtx_;
    };

    ExcutorContext::ExcutorContext() : imp_(std::make_unique<Imp>()) {};
    ExcutorContext::~ExcutorContext() = default;

    TaskRunnerTag ExcutorContext::AddTaskRunner(const TaskRunnerTag& tag) {
        std::lock_guard<std::mutex> lock(imp_->mtx_);
        TaskRunnerTag latest_tag = tag;
        while (imp_->task_runner_dict_.find(latest_tag) != imp_->task_runner_dict_.end()) {
            latest_tag = GetNextRunnerTag_();
        }
        TaskRunnerPtr runner = std::make_unique<TaskRunner>(1);
        runner->Start();
        imp_->task_runner_dict_.emplace(latest_tag, std::move(runner));
        return latest_tag;
    }

    ExcutorContext::TaskRunner* ExcutorContext::GetTaskRunner_(const TaskRunnerTag& tag) {
        std::lock_guard<std::mutex> lock(imp_->mtx_);
        if (imp_->task_runner_dict_.find(tag) == imp_->task_runner_dict_.end()) {
            return nullptr;
        }
        return imp_->task_runner_dict_[tag].get();
    }


}