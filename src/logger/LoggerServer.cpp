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

    // Create UDP socket
    const char* log_port = std::getenv("YIBO_LOG_PORT");
    uint16_t port = log_port ? static_cast<uint16_t>(std::atoi(log_port)) : 9000;

    m_server = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("0.0.0.0", port, 
                                       SOCK_ISSERVER | SOCK_ISUDP | SOCK_ISIP | SOCK_ISREUSE);

    auto result = m_server->Init(param);
    if (result.IsErr()) {
        return result;
    }

    result = m_server->Link(); // For UDP server, this just binds
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
                // For UDP, all data comes through the server socket
                (void)HandleLogData(fd);
            }
        }
    }
}



Result<void, Error> CLoggerServer::HandleLogData(int client_fd) {
    // Receive UDP packet with sender address
    Buffer buffer;
    sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    auto result = m_server->RecvFrom(buffer, 8192, &client_addr, &client_addr_len);

    if (result.IsErr()) {
        return Result<void, Error>::Ok(); // Just ignore errors in UDP
    }

    // Parse RUDP packet: [Seq(4 bytes)][LogInfo data]
    if (buffer.size() < 4) {
        return Result<void, Error>::Ok(); // Invalid packet, ignore
    }

    // Extract sequence number
    uint32_t seq = 0;
    for (int i = 0; i < 4; ++i) {
        seq = (seq << 8) | static_cast<uint8_t>(buffer[i]);
    }

    // Send ACK back to client
    Buffer ack_packet;
    ack_packet.push_back('A');
    ack_packet.push_back('C');
    ack_packet.push_back('K');
    for (int i = 3; i >= 0; --i) {
        ack_packet.push_back(static_cast<char>((seq >> (i * 8)) & 0xFF));
    }
    
    (void)m_server->SendTo(ack_packet, client_addr, client_addr_len);

    // Extract log data (skip the 4-byte seq header)
    BufferView log_data = BufferView(buffer).substr(4);

    // Deserialize and write log
    auto log_result = LogInfo::Deserialize(log_data);
    if (log_result.IsOk()) {
        auto& [info, consumed] = log_result.Value();
        (void)m_writer->Write(info);
    }

    return Result<void, Error>::Ok();
}

} // namespace yibo
