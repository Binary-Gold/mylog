#pragma once

#include <chrono>

#include "using.hpp"

namespace context {
    class ExcutorTimer {
    public:
        struct Internals {
            std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
            Task task;
            RepeatedTaskId repeated_id;

            bool operator<(const Internals& b) const { return time_point > b.time_point; }
        };
        
        ExcutorTimer();
        ~ExcutorTimer();

        ExcutorTimer(const ExcutorTimer&) = delete;
        ExcutorTimer& operator=(const ExcutorTimer&) = delete;

        bool Start();
        void Stop();

        void PostDelayedTask(Task task, const std::chrono::milliseconds& delta);
        RepeatedTaskId PostRepeatedTask(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num);
        void CancleRepeatedTask(RepeatedTaskId task_id);
    private:
        void Run_();
        

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}