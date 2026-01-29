#include "concurrent/ThreadPool.h"

namespace yibo {

CThreadPool::CThreadPool() = default;

CThreadPool::~CThreadPool() {
    Stop();
}

void CThreadPool::Start(size_t thread_count) {
    for (size_t i = 0; i < thread_count; ++i) {
        m_workers.emplace_back([this] {
            WorkerThread();
        });
    }
}

void CThreadPool::Stop() {
    {
        LockGuard<Mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cond.notify_all();

    for (auto& thread : m_workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void CThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;
        {
            UniqueLock<Mutex> lock(m_mutex);
            m_cond.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            if (m_stop && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task();
    }
}

} // namespace yibo
