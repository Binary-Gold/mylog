#pragma once

#include <chrono>

#include "using.hpp"

namespace context {
    class ExcutorTimer {
    public:
        struct InternalS {
            std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
            Task task;
            RepeatedTaskId repeated_id;

            bool operator<(const InternalS& b) const { return time_point > b.time_point; }
        };
        
        ExcutorTimer();
        ~ExcutorTimer();

        ExcutorTimer(const ExcutorTimer&) = delete;
        ExcutorTimer& operator=(const ExcutorTimer&) = delete;

        bool Start();
        void Stop();

        void PostDelayedTask(Task task, const std::chrono::microseconds& delta);
        RepeatedTaskId PostRepeatedTask(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num);
        void CancleRepeatedTask(RepeatedTaskId task_id);
    private:
        void Run_();
        void PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, RepeatedTaskId task_id, uint64_t repeat_num);
        void PostTask_(Task task, const std::chrono::microseconds& delta, RepeatedTaskId task_id, uint64_t repeat_num);
        RepeatedTaskId GetNextRepeatedTaskId();

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}