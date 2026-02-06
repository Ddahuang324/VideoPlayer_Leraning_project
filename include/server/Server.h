#pragma once

#include "business/Business.h"
#include "network/Epoll.h"
#include "concurrent/ThreadPool.h"
#include "concurrent/Process.h"
#include <map>
#include <memory>

namespace yibo {

class CServer {
public:
    CServer() = default;
    ~CServer() = default;

    // 初始化服务器
    Result<void, Error> Init(CBusiness* business);

    // 设置通信进程
    void SetProcess(CProcess* process) { m_process = process; }

    // 运行服务器
    Result<void, Error> Run();

    // 停止服务器
    void Stop();

    // 添加监听套接字
    Result<void, Error> AddListenSocket(SharedPtr<CSocket> socket);

private:
    // 从主进程接收客户端连接
    int ReceiveFD();

    // 处理Epoll事件
    void HandleEvent(const epoll_event& event);

private:
    CBusiness* m_business = nullptr;
    CEpoll m_epoll;
    CThreadPool m_pool;
    CProcess* m_process = nullptr;
    std::map<int, SharedPtr<CSocket>> m_listeners;
    std::map<int, SharedPtr<CSocket>> m_clients;
    Mutex m_mutex;
    bool m_running = false;
};

} // namespace yibo
