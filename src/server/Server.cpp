#include "server/Server.h"
#include "network/Socket.h"
#include <unistd.h>

namespace yibo {

Result<void, Error> CServer::Init(CBusiness* business) {
    if (!business) {
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Business cannot be null"));
    }

    m_business = business;

    // 创建Epoll实例
    auto result = m_epoll.Create(1024);
    if (result.IsErr()) {
        return Result<void, Error>::Err(result.Error());
    }

    // 启动线程池
    size_t thread_count = m_business->GetThreadPoolSize();
    m_pool.Start(thread_count);

    return Result<void, Error>::Ok();
}

Result<void, Error> CServer::Run() {
    m_running = true;

    // 将进程通信FD加入Epoll（如果支持FD传递）
    if (m_process && m_process->GetFd() >= 0) {
        (void)m_epoll.Add(m_process->GetFd(), EpollData(m_process->GetFd()), EPOLLIN);
    }

    // 调用业务进程主循环
    auto result = m_business->BusinessProcess();
    if (result.IsErr()) {
        return Result<void, Error>::Err(result.Error());
    }

    EPEvents events;
    while (m_running) {
        auto wait_result = m_epoll.WaitEvents(events, 10);
        if (wait_result.IsErr()) {
            continue;
        }

        size_t count = wait_result.Value();
        for (size_t i = 0; i < count; ++i) {
            HandleEvent(events[i]);
        }
    }

    return Result<void, Error>::Ok();
}

void CServer::Stop() {
    m_running = false;
    m_pool.Stop();
    (void)m_epoll.Close();
}

Result<void, Error> CServer::AddListenSocket(SharedPtr<CSocket> socket) {
    if (!socket || *socket == -1) {
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Invalid socket"));
    }

    int fd = *socket;
    {
        LockGuard<Mutex> lock(m_mutex);
        m_listeners[fd] = socket;
    }

    return m_epoll.Add(fd, EpollData(fd), EPOLLIN);
}

int CServer::ReceiveFD() {
    if (!m_process) {
        return -1;
    }

    int client_fd = -1;
    auto result = m_process->RecvFD(client_fd);
    if (result.IsErr()) {
        return -1;
    }

    return client_fd;
}

void CServer::HandleEvent(const epoll_event& event) {
    int fd = event.data.fd;

    // 如果是进程间通信FD，说明有新的连接进来
    if (m_process && fd == m_process->GetFd()) {
        int client_fd = ReceiveFD();
        if (client_fd >= 0) {
            auto client = MakeShared<CSocket>();
            if (client->InitFromExisting(client_fd).IsOk()) {
                {
                    LockGuard<Mutex> lock(m_mutex);
                    m_clients[client_fd] = client;
                }
                (void)m_epoll.Add(client_fd, EpollData(client_fd), EPOLLIN);
                (void)m_business->Connected(client.get());
            } else {
                ::close(client_fd);
            }
        }
        return;
    }

    // 检查是否是监听套接字
    SharedPtr<CSocket> listener;
    {
        LockGuard<Mutex> lock(m_mutex);
        auto it = m_listeners.find(fd);
        if (it != m_listeners.end()) {
            listener = it->second;
        }
    }

    if (listener) {
        auto accept_result = listener->Accept();
        if (accept_result.IsOk()) {
            auto client_sock = std::move(accept_result.Value());
            int client_fd = *client_sock;
            SharedPtr<CSocket> client_ptr = std::move(client_sock); 
            {
                LockGuard<Mutex> lock(m_mutex);
                m_clients[client_fd] = client_ptr;
            }
            (void)m_epoll.Add(client_fd, EpollData(client_fd), EPOLLIN);
            (void)m_business->Connected(client_ptr.get());
        }
        return;
    }

    // 查找客户端连接
    SharedPtr<CSocket> client;
    {
        LockGuard<Mutex> lock(m_mutex);
        auto it = m_clients.find(fd);
        if (it == m_clients.end()) {
            return;
        }
        client = it->second;
    }

    // 提交到线程池处理
    m_pool.AddTask([this, client, fd]() {
        Buffer data;
        auto result = client->Recv(data, 8192);
        if (result.IsErr() || data.empty()) {
            // 连接关闭或错误
            (void)m_epoll.Del(fd);
            {
                LockGuard<Mutex> lock(m_mutex);
                auto it = m_clients.find(fd);
                if (it != m_clients.end() && it->second == client) {
                    m_clients.erase(it);
                }
            }
            return;
        }

        // 调用业务逻辑处理
        (void)m_business->Received(client.get(), data);
    });
}

} // namespace yibo
