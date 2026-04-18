#pragma once
#include <chrono>

#include "using.hpp"
#include "excutor_context.hpp"

namespace context{
    class Excutor
    {
    public:
        Excutor();
        ~Excutor();

        Excutor(const Excutor&) = delete;
        Excutor& operator=(const Excutor&) = delete;

        TaskRunnerTag AddTaskRunner(const TaskRunnerTag& tag);
        void PostTask(const TaskRunnerTag& tag, Task task);
        
        template<typename R, typename P>
        void PostDelayedTask(const TaskRunnerTag& tag, Task task, const std::chrono::duration<R, P>& delta) {
            Task func = std::bind<std::function<void>>(&Excutor::PostTask, this, tag, std::move(task));
            PostDelayedTask_(func, std::chrono::duration_cast<std::chrono::microseconds>(delta));
        }

        template<typename R, typename P>
        RepeatedTaskId PostRepeatedTask(const TaskRunnerTag& tag, Task task, const std::chrono::duration<R, P>& delta, uint64_t repeat_num) {
            Task func = std::bind<std::function<void>>(&Excutor::PostTask, this, tag, std::move(task));
            return PostRepeatedTask_(func, std::chrono::duration_cast<std::chrono::microseconds>(delta), repeat_num);
        }
        void CanCelRepeatedTask(RepeatedTaskId task_id);

        template<typename F, typename... Args>
        auto PostTaskAndGetResult(const TaskRunnerTag& tag, F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            ExcutorContext::TaskRunner* task_runner = GetTaskRunner_(tag);
            auto ret = task_runner->RunRetTask(std::forward<F>(f), std::forward<Args>(args)...);
            return ret;
        }
    private:
        void PostDelayedTask_(Task task, const std::chrono::microseconds& delta);
        RepeatedTaskId PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num);
        ExcutorContext::TaskRunner* GetTaskRunner_(const TaskRunnerTag& tag);
        
        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
    
}