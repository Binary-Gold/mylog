#include <queue>
#include <condition_variable>
#include <unordered_set>

#include "context/excutor.hpp"
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
            if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 0) {
                imp_->cond_.wait_for(lock, diff);
                continue;
            } else {
                imp_->queue_.pop();
                lock.unlock();
                s.task();
            }
        }
        
    }
}