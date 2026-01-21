#pragma once
#include "common/Result.h"
#include "common/Error.h"
#include "network/EpollData.h"
#include <vector>
#include <sys/epoll.h>

namespace yibo {

using EPEvents = std::vector<epoll_event>;

class CEpoll {
public:
    CEpoll() : m_epoll(-1), m_events(1024) {}

    ~CEpoll() { Close(); }

    CEpoll(const CEpoll&) = delete;
    CEpoll& operator=(const CEpoll&) = delete;

    CEpoll(CEpoll&& other) noexcept
        : m_epoll(std::exchange(other.m_epoll, -1))
        , m_events(std::move(other.m_events)) {}

    CEpoll& operator=(CEpoll&& other) noexcept {
        if (this != &other) {
            Close();
            m_epoll = std::exchange(other.m_epoll, -1);
            m_events = std::move(other.m_events);
        }
        return *this;
    }

    Result<void, Error> Create(unsigned count);
    Result<void, Error> Add(int fd, const EpollData& data, uint32_t events);
    Result<void, Error> Modify(int fd, uint32_t events, const EpollData& data);
    Result<void, Error> Del(int fd);
    Result<size_t, Error> WaitEvents(EPEvents& events, int timeout = 10);
    void Close();

    operator int() const { return m_epoll; }

private:
    int m_epoll;
    EPEvents m_events;
};

} // namespace yibo
