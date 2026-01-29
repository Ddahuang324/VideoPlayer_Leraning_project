#pragma once
#include <sys/epoll.h>
#include <cstdint>

namespace yibo {

class EpollData {
public:
    EpollData() { m_data.fd = -1; }

    explicit EpollData(int fd) { m_data.fd = fd; }
    explicit EpollData(void* ptr) { m_data.ptr = ptr; }
    explicit EpollData(uint32_t u32) { m_data.u32 = u32; }
    explicit EpollData(uint64_t u64) { m_data.u64 = u64; }

    operator int() const { return m_data.fd; }
    operator void*() const { return m_data.ptr; }
    operator uint32_t() const { return m_data.u32; }
    operator uint64_t() const { return m_data.u64; }

    epoll_data_t& Data() { return m_data; }
    const epoll_data_t& Data() const { return m_data; }

private:
    epoll_data_t m_data;
};

} // namespace yibo
