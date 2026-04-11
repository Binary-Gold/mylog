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
    
    ThreadPool::ThreadPool(uint32_t thread_count) : imp_(std::make_unique<Imp>()) {
        imp_->is_available_.store(false);
        imp_->is_shutdown_.store(false);
        imp_->thread_count_.store(thread_count);
    }
    ThreadPool::~ThreadPool() {
        Stop();
    }

    void ThreadPool::EnqueueTask_(Task task) {
        {
            std::unique_lock<std::mutex> lock(imp_->mtx_);
            imp_->tasks_.emplace(std::move(task));
        }
        imp_->task_cv_.notify_one();
    }

    bool ThreadPool::Start() {
        if (imp_->is_available_.load()) {
            return false;
        }
        imp_->is_shutdown_.store(false);
        imp_->is_available_.store(true);
        uint32_t thread_count = imp_->thread_count_.load();
        for (uint32_t i = 0; i < thread_count; ++i) {
            AddThread();
        }
        return true;
    }
    void ThreadPool::Stop() {
        if (imp_->is_available_.load()) {
            imp_->is_shutdown_.store(true);
            imp_->task_cv_.notify_all();
            imp_->is_available_.store(false);
        }
        imp_->workers_.clear();
    }

    void ThreadPool::AddThread() {
        auto func = [this]() {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(imp_->mtx_);
                    imp_->task_cv_.wait(lock, [this]() {
                        return imp_->is_shutdown_.load() || !imp_->tasks_.empty();
                    });
                    if (imp_->is_shutdown_.load()) {
                        break;
                    }
                    task = std::move(imp_->tasks_.front());
                    imp_->tasks_.pop();
                }
                if (task) {
                    task();
                }
            }
        };
        ThreadPtr th = std::make_shared<std::thread>(std::move(func));
        imp_->workers_.emplace_back(std::make_shared<ThreadInfo>(th));
    }

    ThreadPool::ThreadInfo::~ThreadInfo() {
        if (ptr && ptr->joinable()) {
            ptr->join();
        }
    }
}