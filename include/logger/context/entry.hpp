#pragma once

#include "logger/context/context.hpp"
#include "logger/context/using.hpp"

using namespace logger::ctx;

inline Context* CONTEXT() {
    return Context::GetInstance();
}

inline Executor* EXECUTOR() {
    return CONTEXT()->GetExecutor();
}

inline TaskRunnerTag NEW_TASK_RUNNER(TaskRunnerTag tag) {
    return CONTEXT()->NewTaskRunner(tag);
}

inline void POST_TASK(const TaskRunnerTag& tag, Task task) {
    EXECUTOR()->PostTask(tag, task);
}

// 因为池是单线程,投递顺序就是执行顺序
inline void WAIT_TASK_IDLE(const TaskRunnerTag& tag) {
    EXECUTOR()->PostTaskAndGetResult(tag, [](){})->wait();
}

template<typename R, typename P>
inline RepeatedTaskId POST_REPEATED_TASK(
    const TaskRunnerTag& tag, Task task,
    const std::chrono::duration<R, P>& delta, uint64_t repeat_num) {
  return EXECUTOR()->PostRepeatedTask(tag, std::move(task), delta, repeat_num);
}
