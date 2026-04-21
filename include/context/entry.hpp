#pragma once

#include "context/context.hpp"
#include "context/using.hpp"

using namespace context;

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

inline void WAIT_TASK_IDLE(const TaskRunnerTag& tag) {
    EXECUTOR()->PostTaskAndGetResult(tag, [](){}).wait();
}

template<typename R, typename P>
inline RepeatedTaskId POST_REPEATED_TASK(
    const TaskRunnerTag& tag, Task task,
    const std::chrono::duration<R, P>& delta, uint64_t repeat_num) {
  return EXECUTOR()->PostRepeatedTask(tag, std::move(task), delta, repeat_num);
}
