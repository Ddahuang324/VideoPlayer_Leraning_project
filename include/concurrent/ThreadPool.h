#pragma once

#include "common/Public.h"
#include "concurrent/Thread.h"
#include <queue>
#include <functional>
#include <future>
#include <atomic>

namespace yibo {

/**
 * @brief Modern C++17 thread pool implementation
 *
 * Features:
 * - Type-safe task submission with std::function
 * - Return value support via std::future
 * - Exception propagation through futures
 * - Graceful shutdown mechanism
 * - RAII resource management
 */
class CThreadPool {
public:
    CThreadPool();
    ~CThreadPool();

    // Disable copy
    CThreadPool(const CThreadPool&) = delete;
    CThreadPool& operator=(const CThreadPool&) = delete;

    // Enable move
    CThreadPool(CThreadPool&&) = default;
    CThreadPool& operator=(CThreadPool&&) = default;

    /**
     * @brief Start the thread pool with specified number of worker threads
     * @param thread_count Number of worker threads to create
     */
    void Start(size_t thread_count);

    /**
     * @brief Stop the thread pool gracefully (wait for all tasks to complete)
     */
    void Stop();

    /**
     * @brief Submit a task to the thread pool
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param func Function to execute
     * @param args Arguments to pass to the function
     * @return std::future to retrieve the result
     */
    template<typename Func, typename... Args>
    auto AddTask(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>;

private:
    /**
     * @brief Worker thread main loop
     */
    void WorkerThread();

    std::vector<std::thread> m_workers;                  // Worker threads
    std::queue<std::function<void()>> m_tasks;           // Task queue
    Mutex m_mutex;                                       // Protects task queue
    ConditionVariable m_cond;                            // Notifies workers
    std::atomic<bool> m_stop{false};                     // Stop flag
};

// Template implementation
template<typename Func, typename... Args>
auto CThreadPool::AddTask(Func&& func, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...>>
{
    using ReturnType = std::invoke_result_t<Func, Args...>;

    // Create packaged_task
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        LockGuard<Mutex> lock(m_mutex);

        // Don't allow adding tasks after stop
        if (m_stop) {
            throw std::runtime_error("Cannot add task to stopped thread pool");
        }

        m_tasks.emplace([task]() { (*task)(); });
    }

    m_cond.notify_one();
    return result;
}

} // namespace yibo
