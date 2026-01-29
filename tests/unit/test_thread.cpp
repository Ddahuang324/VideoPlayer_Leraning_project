#include <gtest/gtest.h>
#include "concurrent/Thread.h"
#include <atomic>
#include <chrono>
#include <set>

using namespace yibo;

TEST(ThreadTest, BasicExecution) {
    std::atomic<bool> executed{false};
    {
        CThread thread([&executed]() {
            executed = true;
        });
    }
    EXPECT_TRUE(executed.load());
}

TEST(ThreadTest, WithArguments) {
    std::atomic<int> result{0};
    {
        CThread thread([](std::atomic<int>& r, int a, int b) {
            r = a + b;
        }, std::ref(result), 10, 20);
    }
    EXPECT_EQ(result.load(), 30);
}

TEST(ThreadTest, MoveSemantics) {
    CThread thread1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    CThread thread2 = std::move(thread1);
    EXPECT_FALSE(thread1.Joinable());
    EXPECT_TRUE(thread2.Joinable());
}

TEST(ThreadTest, DelayedStart) {
    std::atomic<bool> executed{false};
    CThread thread;
    EXPECT_FALSE(thread.Joinable());

    thread.Start([&executed]() {
        executed = true;
    });
    EXPECT_TRUE(thread.Joinable());

    thread.Join();
    EXPECT_TRUE(executed.load());
}

TEST(ThreadTest, GetId) {
    std::thread::id thread_id;
    CThread thread([&thread_id]() {
        thread_id = std::this_thread::get_id();
    });
    thread.Join();
    EXPECT_NE(thread_id, std::thread::id());
}

TEST(ThreadTest, HardwareConcurrency) {
    unsigned int cores = CThread::HardwareConcurrency();
    EXPECT_GT(cores, 0u);
}

TEST(MutexTest, BasicLocking) {
    Mutex mtx;
    int shared_data = 0;

    std::vector<CThread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&mtx, &shared_data]() {
            for (int j = 0; j < 1000; ++j) {
                LockGuard<Mutex> lock(mtx);
                shared_data++;
            }
        });
    }

    threads.clear();
    EXPECT_EQ(shared_data, 10000);
}

TEST(ConditionVariableTest, WaitAndNotify) {
    Mutex mtx;
    ConditionVariable cv;
    bool ready = false;

    CThread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            LockGuard<Mutex> lock(mtx);
            ready = true;
        }
        cv.notify_one();
    });

    UniqueLock<Mutex> lock(mtx);
    cv.wait(lock, [&]{ return ready; });
    EXPECT_TRUE(ready);
}

TEST(ThreadLocalTest, BasicUsage) {
    thread_local int tls_value = 0;

    std::vector<CThread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i, &success_count]() {
            tls_value = i;
            if (tls_value == i) {
                success_count++;
            }
        });
    }

    threads.clear();
    EXPECT_EQ(success_count.load(), 5);
}
