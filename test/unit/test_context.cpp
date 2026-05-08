#include <gtest/gtest.h>

#include <future>
#include <atomic>
#include <thread>

#include "logger/context/entry.hpp"

namespace {

using namespace logger;

TEST(ContextTest, CONTEXT) {
    logger::ctx::Context* p1 = CONTEXT();
    logger::ctx::Context* p2 = CONTEXT();

    EXPECT_EQ(p1, p2);
}

TEST(ContextTest, EXECUTOR) {
    logger::ctx::Executor* p1 = EXECUTOR();
    logger::ctx::Executor* p2 = EXECUTOR();

    EXPECT_EQ(p1, p2);
}

TEST(ContextTest, NEW_TASK_RUNNER) {
    logger::ctx::TaskRunnerTag p1 = NEW_TASK_RUNNER(0);
    logger::ctx::TaskRunnerTag p2 = NEW_TASK_RUNNER(0);

    EXPECT_EQ(p1, 0);
    EXPECT_EQ(p2, 1);
}

// ==================== POST_TASK ====================

TEST(ContextTest, PostTaskRunsAsync) {
    std::promise<void> done;
    auto signal = done.get_future();
    auto tag = NEW_TASK_RUNNER(66666);

    POST_TASK(tag, [&done]() { done.set_value(); });

    auto status = signal.wait_for(std::chrono::seconds(3));
    EXPECT_EQ(status, std::future_status::ready);
}

TEST(ContextTest, PostTaskRunsInDifferentThread) {
    std::promise<std::thread::id> done;
    auto signal = done.get_future();
    auto tag = NEW_TASK_RUNNER(77777);

    POST_TASK(tag, [&done]() { done.set_value(std::this_thread::get_id()); });

    auto status = signal.wait_for(std::chrono::seconds(3));
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_NE(signal.get(), std::this_thread::get_id());
}

TEST(ContextTest, PostTaskMultipleTasksAllRun) {
    std::atomic<int> counter{0};
    constexpr int kTaskCount = 1000;
    auto tag = NEW_TASK_RUNNER(88888);

    for (int i = 0; i < kTaskCount; ++i) {
        POST_TASK(tag, [&counter]() { counter.fetch_add(1); });
    }

    WAIT_TASK_IDLE(tag);
    EXPECT_EQ(counter.load(), kTaskCount);
}

// ==================== WAIT_TASK_IDLE ====================

TEST(ContextTest, WaitTaskIdleBlocksUntilTasksDone) {
    std::atomic<int> counter{0};
    auto tag = NEW_TASK_RUNNER(99999);

    // 投 5 个任务，全部放进队列后才 WAIT_TASK_IDLE
    for (int i = 0; i < 5; ++i) {
        POST_TASK(tag, [&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            counter.fetch_add(1);
        });
    }
    WAIT_TASK_IDLE(tag);

    EXPECT_EQ(counter.load(), 5);  // 等完全部 5 个任务
}

// ====================  ====================
TEST(ContextTest, POST_REPEATED_TASK) {
    std::atomic<int> counter{0};
    auto tag = NEW_TASK_RUNNER(12345);
    POST_REPEATED_TASK(tag, [&counter]() {
        counter.fetch_add(1);
    },std::chrono::milliseconds(10), 10);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(counter.load(), 10);
}
}  // namespace
