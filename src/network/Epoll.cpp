#include "network/Epoll.h"
#include "common/ErrorCode.h"
#include <unistd.h>
#include <cstring>

namespace yibo {

Result<void, Error> CEpoll::Create(unsigned count) {
    if (m_epoll != -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidState, "Epoll already created"));
    }

    m_epoll = epoll_create(count);
    if (m_epoll == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::NetworkError,
                  std::string("epoll_create failed: ") + strerror(errno)));
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CEpoll::Add(int fd, const EpollData& data, uint32_t events) {
    if (m_epoll == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidState, "Epoll not initialized"));
    }

    epoll_event ev;
    ev.events = events;
    ev.data = data.Data();

    if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &ev) == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::NetworkError,
                  std::string("epoll_ctl ADD failed: ") + strerror(errno)));
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CEpoll::Modify(int fd, uint32_t events, const EpollData& data) {
    if (m_epoll == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidState, "Epoll not initialized"));
    }

    epoll_event ev;
    ev.events = events;
    ev.data = data.Data();

    if (epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::NetworkError,
                  std::string("epoll_ctl MOD failed: ") + strerror(errno)));
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CEpoll::Del(int fd) {
    if (m_epoll == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidState, "Epoll not initialized"));
    }

    if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        return Result<void, Error>::Err(
            Error(ErrorCode::NetworkError,
                  std::string("epoll_ctl DEL failed: ") + strerror(errno)));
    }

    return Result<void, Error>::Ok();
}

Result<size_t, Error> CEpoll::WaitEvents(EPEvents& events, int timeout) {
    if (m_epoll == -1) {
        return Result<size_t, Error>::Err(
            Error(ErrorCode::InvalidState, "Epoll not initialized"));
    }

    if (events.empty()) {
        events.resize(m_events.size());
    }

    int nfds = epoll_wait(m_epoll, events.data(),
                          static_cast<int>(events.size()), timeout);

    if (nfds == -1) {
        if (errno == EINTR) {
            return Result<size_t, Error>::Ok(0);
        }
        return Result<size_t, Error>::Err(
            Error(ErrorCode::NetworkError,
                  std::string("epoll_wait failed: ") + strerror(errno)));
    }

    if (static_cast<size_t>(nfds) == events.size() && events.size() < 10240) {
        m_events.resize(events.size() * 2);
    }

    return Result<size_t, Error>::Ok(static_cast<size_t>(nfds));
}

void CEpoll::Close() {
    if (m_epoll != -1) {
        close(m_epoll);
        m_epoll = -1;
    }
}

} // namespace yibo
