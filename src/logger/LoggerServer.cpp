#include "logger/LoggerServer.h"
#include "network/EpollData.h"
#include <filesystem>
#include <unistd.h>

namespace yibo {

Result<void, Error> CLoggerServer::Start(const std::string& log_dir) {
    if (m_running) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidState, "Logger server already running"));
    }

    // Create log directory
    std::filesystem::create_directories(log_dir);

    // Create writer
    m_writer = MakeUnique<LogWriter>(log_dir);

    // Create Unix Domain Socket
    m_socket_path = log_dir + "/server.sock";
    unlink(m_socket_path.c_str());

    m_server = MakeUnique<CSocket>();
    auto param = CSockParam::MakeUnix(m_socket_path, SOCK_ISSERVER | SOCK_ISNONBLOCK);

    auto result = m_server->Init(param);
    if (result.IsErr()) {
        return result;
    }

    result = m_server->Link();
    if (result.IsErr()) {
        return result;
    }

    // Create epoll
    result = m_epoll.Create(1024);
    if (result.IsErr()) {
        return result;
    }

    // Add server socket to epoll
    EpollData data(static_cast<int>(*m_server));
    result = m_epoll.Add(static_cast<int>(*m_server), data, EPOLLIN | EPOLLET);
    if (result.IsErr()) {
        return result;
    }

    // Start event loop thread
    m_running = true;
    m_thread = std::thread(&CLoggerServer::EventLoop, this);

    return Result<void, Error>::Ok();
}

Result<void, Error> CLoggerServer::Stop() {
    if (!m_running) {
        return Result<void, Error>::Ok();
    }

    m_running = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_clients.clear();
    m_server.reset();
    m_epoll.Close();
    m_writer.reset();

    if (!m_socket_path.empty()) {
        unlink(m_socket_path.c_str());
    }

    return Result<void, Error>::Ok();
}

void CLoggerServer::EventLoop() {
    EPEvents events;

    while (m_running) {
        auto result = m_epoll.WaitEvents(events, 1000);

        if (result.IsErr()) {
            continue;
        }

        size_t count = result.Value();
        for (size_t i = 0; i < count; ++i) {
            int fd = events[i].data.fd;

            if (fd == static_cast<int>(*m_server)) {
                (void)HandleNewConnection();
            } else {
                (void)HandleLogData(fd);
            }
        }
    }
}

Result<void, Error> CLoggerServer::HandleNewConnection() {
    while (true) {
        auto result = m_server->Accept();

        if (result.IsErr()) {
            break;
        }

        auto client = std::move(result.Value());
        int client_fd = static_cast<int>(*client);

        EpollData data(client_fd);
        (void)m_epoll.Add(client_fd, data, EPOLLIN | EPOLLET);

        m_clients[client_fd] = std::move(client);
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CLoggerServer::HandleLogData(int client_fd) {
    auto it = m_clients.find(client_fd);
    if (it == m_clients.end()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Client not found"));
    }

    Buffer buffer;
    auto result = it->second->Recv(buffer, 8192);

    if (result.IsErr() || result.Value() == 0) {
        (void)m_epoll.Del(client_fd);
        m_clients.erase(it);
        return Result<void, Error>::Ok();
    }

    // Process all log messages in the buffer
    BufferView data = buffer;
    while (!data.empty()) {
        auto log_result = LogInfo::Deserialize(data);
        if (log_result.IsOk()) {
            auto& [info, consumed] = log_result.Value();
            (void)m_writer->Write(info);
            
            if (consumed > 0 && consumed <= data.size()) {
                data = data.substr(consumed);
            } else {
                break;
            }
        } else {
            // If we can't deserialize, it might be partial data.
            // For now, in this simple implementation, we assume packets are complete or we drop.
            // A more robust implementation would need a buffer per client.
            break;
        }
    }

    return Result<void, Error>::Ok();
}

} // namespace yibo
