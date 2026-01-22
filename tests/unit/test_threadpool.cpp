#include "concurrent/ThreadPool.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

using namespace yibo;

TEST(ThreadPoolTest, BasicTaskExecution) {
    CThreadPool pool;
    pool.Start(2);

    std::atomic<int> counter{0};
    pool.AddTask([&counter]() {
        counter++;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, TaskWithReturnValue) {
    CThreadPool pool;
    pool.Start(2);

    auto future = pool.AddTask([](int a, int b) {
        return a + b;
    }, 10, 20);

    EXPECT_EQ(future.get(), 30);
}

TEST(ThreadPoolTest, ExceptionPropagation) {
    CThreadPool pool;
    pool.Start(2);

    auto future = pool.AddTask([]() {
        throw std::runtime_error("test error");
        return 42;
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPoolTest, ConcurrentTasks) {
    CThreadPool pool;
    pool.Start(4);

    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.AddTask([&counter]() {
            counter++;
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, GracefulShutdown) {
    CThreadPool pool;
    pool.Start(2);

    std::atomic<int> completed{0};
    for (int i = 0; i < 10; ++i) {
        pool.AddTask([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            completed++;
        });
    }

    pool.Stop();
    EXPECT_EQ(completed.load(), 10);
}

TEST(ThreadPoolTest, CannotAddTaskAfterStop) {
    CThreadPool pool;
    pool.Start(2);
    pool.Stop();

    EXPECT_THROW(
        pool.AddTask([]() { return 42; }),
        std::runtime_error
    );
}
