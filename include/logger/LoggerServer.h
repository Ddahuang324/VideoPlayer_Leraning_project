#pragma once

#include "logger/LogWriter.h"
#include "network/Epoll.h"
#include "network/Socket.h"
#include "common/Result.h"
#include "common/Error.h"
#include "common/Public.h"
#include <thread>
#include <atomic>
#include <unordered_map>

namespace yibo {

class CLoggerServer {
public:
    static CLoggerServer& Instance() {
        static CLoggerServer instance;
        return instance;
    }

    CLoggerServer(const CLoggerServer&) = delete;
    CLoggerServer& operator=(const CLoggerServer&) = delete;

    Result<void, Error> Start(const std::string& log_dir = "./log");
    Result<void, Error> Stop();

private:
    CLoggerServer() = default;
    ~CLoggerServer() { (void)Stop(); }

    void EventLoop();
    Result<void, Error> HandleLogData(int client_fd);

    std::thread m_thread;
    CEpoll m_epoll;
    UniquePtr<CSocket> m_server;
    UniquePtr<LogWriter> m_writer;
    std::unordered_map<int, UniquePtr<CSocket>> m_clients;
    std::atomic<bool> m_running{false};
};

} // namespace yibo

