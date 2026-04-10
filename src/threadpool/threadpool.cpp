#include "threadpool/threadpool.hpp"

namespace threadpool{
    struct ThreadPool::Imp {
        std::vector<ThreadInfoPtr> workers_;
        std::queue<Task> tasks_;

        std::mutex mtx_;
        std::condition_variable task_cv_;

        std::atomic<uint32_t> thread_count_;
        std::atomic<bool> is_shutdown_;
        std::atomic<bool> is_available_; 
    };
    
    ThreadPool::ThreadPool(uint32_t thread_count) {
        imp_->is_available_.store(false);
        imp_->is_shutdown_.store(false);
        imp_->thread_count_.store(thread_count);
    }
    ThreadPool::~ThreadPool() {
        Stop();
    }

    void ThreadPool::Stop() {
        if (imp_->is_available_.load()) {
            imp_->is_shutdown_.store(true);
            imp_->task_cv_.notify_all();
        }
        imp_->workers_.clear();
    }
    void ThreadPool::AddThread() {
        auto func = [this](){
            while(1) {
                Task task;
                {
                    std::unique_lock lock(imp_->mtx_);
                    
                }
            }
        };
    }

    ThreadPool::ThreadInfo::~ThreadInfo() {
        if (ptr && ptr->joinable()) {
            ptr->join();
        }
    }
}