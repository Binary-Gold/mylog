#pragma once

#include <memory>

#include "using.hpp"
#include "excutor_timer.hpp"
#include "threadpool.hpp"

namespace context {
    class ExcutorContext {
    public:
        ExcutorContext();
        ~ExcutorContext();

        ExcutorContext(const ExcutorContext&) = delete;
        ExcutorContext& operator=(const ExcutorContext&) = delete;

        TaskRunnerTag AddTaskRunner(const TaskRunnerTag& tag);
    private:
        using TaskRunner = ThreadPool;
        using TaskRunnerPtr = std::unique_ptr<TaskRunner>;
        friend class Excutor;

        TaskRunner* GetTaskRunner_(const TaskRunnerTag& tag);
        TaskRunnerTag GetNextRunnerTag_();

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}