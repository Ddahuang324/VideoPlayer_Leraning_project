#ifndef YIBO_CONCURRENT_THREAD_H
#define YIBO_CONCURRENT_THREAD_H

#include "common/Public.h"
#include <thread>
#include <utility>

namespace yibo {

// CThread: RAII-style thread wrapper
class CThread {
public:
    CThread() = default;

    template<typename Func, typename... Args>
    explicit CThread(Func&& func, Args&&... args)
        : m_thread(std::forward<Func>(func), std::forward<Args>(args)...)
        , m_started(true) {}

    ~CThread() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    CThread(const CThread&) = delete;
    CThread& operator=(const CThread&) = delete;

    CThread(CThread&& other) noexcept
        : m_thread(std::move(other.m_thread))
        , m_started(std::exchange(other.m_started, false)) {}

    CThread& operator=(CThread&& other) noexcept {
        if (this != &other) {
            if (m_thread.joinable()) {
                m_thread.join();
            }
            m_thread = std::move(other.m_thread);
            m_started = std::exchange(other.m_started, false);
        }
        return *this;
    }

    template<typename Func, typename... Args>
    void Start(Func&& func, Args&&... args) {
        if (m_started) {
            throw std::runtime_error("Thread already started");
        }
        m_thread = std::thread(std::forward<Func>(func), std::forward<Args>(args)...);
        m_started = true;
    }

    void Join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void Detach() {
        if (m_thread.joinable()) {
            m_thread.detach();
        }
    }

    bool Joinable() const {
        return m_thread.joinable();
    }

    std::thread::id GetId() const {
        return m_thread.get_id();
    }

    static unsigned int HardwareConcurrency() {
        return std::thread::hardware_concurrency();
    }

private:
    std::thread m_thread;
    bool m_started = false;
};

} // namespace yibo

#endif // YIBO_CONCURRENT_THREAD_H
