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

    }
}