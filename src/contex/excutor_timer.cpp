#include <queue>
#include <condition_variable>
#include <unordered_set>

#include "context/excutor_timer.hpp"
#include "context/threadpool.hpp"

namespace context {
    struct ExcutorTimer::Imp {
        std::priority_queue<InternalS> queue_;
        std::mutex mtx_;
        std::condition_variable cond_;
        std::atomic<bool> running_;
        std::unique_ptr<ThreadPool> thread_pool_;
        std::atomic<RepeatedTaskId> repeated_task_id_;
        std::unordered_set<RepeatedTaskId> repeated_id_state_set_;
    };

    ExcutorTimer::ExcutorTimer() :imp_(std::make_unique<Imp>()) {
        imp_->thread_pool_ = std::make_unique<ThreadPool>(1);
        imp_->repeated_task_id_.store(0);
        imp_->running_.store(false);
    };
    ExcutorTimer::~ExcutorTimer() {
        Stop();
    }

    bool ExcutorTimer::Start() {
        if (imp_->running_.load()) {
            return true;
        }
        imp_->running_.store(true);
        bool ret = imp_->thread_pool_->Start();
        imp_->thread_pool_->RunTask(&context::ExcutorTimer::Run_, this);
        return ret;
    }
    void ExcutorTimer::Stop() {
        imp_->running_.store(false);
        imp_->cond_.notify_all();
        imp_->thread_pool_.reset();
    }

    void ExcutorTimer::Run_() {
        while (imp_->running_.load())
        {
            std::unique_lock lock(imp_->mtx_);
            imp_->cond_.wait(lock, [this](){ return !this->imp_->queue_.empty(); });
            auto s = imp_->queue_.top();
            auto diff = s.time_point - std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::microseconds>(diff).count() > 0) {
                imp_->cond_.wait_for(lock, diff);
                continue;
            } else {
                imp_->queue_.pop();
                lock.unlock();
                s.task();
            }
        }
    }

    void ExcutorTimer::PostDelayedTask(Task task, const std::chrono::microseconds& delta) {
        InternalS s;
        s.time_point = std::chrono::high_resolution_clock::now() + delta;
        s.task = std::move(task);
        {
            std::lock_guard<std::mutex> lock(imp_->mtx_);
            imp_->queue_.pop();
            imp_->cond_.notify_all();
        }
    }

    RepeatedTaskId ExcutorTimer::PostRepeatedTask(Task task, const std::chrono::microseconds& delta, uint64_t repeat_num) {
        RepeatedTaskId id = GetNextRepeatedTaskId();
        imp_->repeated_id_state_set_.insert(id);
        PostRepeatedTask_(std::move(task), delta, id, repeat_num);
        return id;
    }

    void ExcutorTimer::PostRepeatedTask_(Task task, const std::chrono::microseconds& delta, RepeatedTaskId task_id, uint64_t repeat_num) {
        if (imp_->repeated_id_state_set_.find(task_id) == imp_->repeated_id_state_set_.end() || repeat_num == 0) {
            return;
        }
        task();
        Task func = std::bind(&ExcutorTimer::PostRepeatedTask_, this, std::move(task), delta, task_id, repeat_num - 1);
        InternalS s;
        s.time_point = std::chrono::high_resolution_clock::now() + delta;
        s.repeated_id = imp_->repeated_task_id_;
        s.task = std::move(task);
        {
            std::lock_guard<std::mutex> lock(imp_->mtx_);
            imp_->queue_.pop();
            imp_->cond_.notify_all();
        }
    }

    void ExcutorTimer::CancleRepeatedTask(RepeatedTaskId task_id) {
        imp_->repeated_id_state_set_.erase(task_id);
    }

    

}