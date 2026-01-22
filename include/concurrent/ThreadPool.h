#pragma once

#include "common/Public.h"
#include "concurrent/Thread.h"
#include <queue>
#include <functional>
#include <future>
#include <atomic>

namespace yibo {

class CThreadPool {
public:
    CThreadPool();
    ~CThreadPool();

    // 禁用拷贝
    CThreadPool(const CThreadPool&) = delete;
    CThreadPool& operator=(const CThreadPool&) = delete;

    // 启用移动
    CThreadPool(CThreadPool&&) = default;
    CThreadPool& operator=(CThreadPool&&) = default;

    /**
     * @brief 使用指定的线程数启动线程池
     * @param thread_count 要创建的工作线程数量
     */
    void Start(size_t thread_count);

    /**
     * @brief 优雅地停止线程池（等待所有任务完成）
     */
    void Stop();

    /**
     * @brief 向线程池提交一个任务
     * @tparam Func 函数类型
     * @tparam Args 参数类型
     * @param func 要执行的函数
     * @param args 传递给函数的参数
     * @return 用于获取结果的 std::future
     */
    template<typename Func, typename... Args>
    auto AddTask(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>;

private:
    /**
     * @brief 工作线程主循环
     */
    void WorkerThread();

    std::vector<std::thread> m_workers;                  // 工作线程
    std::queue<std::function<void()>> m_tasks;           // 任务队列
    Mutex m_mutex;                                       // 保护任务队列的互斥锁
    ConditionVariable m_cond;                            // 通知工作线程的条件变量
    std::atomic<bool> m_stop{false};                     // 停止标志
};


template<typename Func, typename... Args>
auto CThreadPool::AddTask(Func&& func, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...>>
{
    using ReturnType = std::invoke_result_t<Func, Args...>;

    // 创建 packaged_task
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)//闭包
    );

    std::future<ReturnType> result = task->get_future();

    {
        LockGuard<Mutex> lock(m_mutex);

        // 停止后不允许添加任务
        if (m_stop) {
            throw std::runtime_error("Cannot add task to stopped thread pool");
        }

        m_tasks.emplace([task]() { (*task)(); });
    }

    m_cond.notify_one();
    return result;
}

} 
