#include "logger/Logger.h"
#include "network/Socket.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

namespace yibo {

// RUDP packet header
struct RUDPPacket {
    uint32_t seq;
    Buffer data;
};

// ACK packet structure
struct ACKPacket {
    char tag[3]; // "ACK"
    uint32_t seq;
};

void Trace(const LogInfo& info) {
    static thread_local CSocket g_log_client;
    static thread_local bool g_initialized = false;
    static thread_local uint32_t g_seq = 0;
    static thread_local sockaddr_storage g_server_addr;
    static thread_local socklen_t g_server_addr_len;

    // Lazy initialization
    if (!g_initialized) {
        const char* log_ip = std::getenv("YIBO_LOG_IP");
        std::string ip = log_ip ? log_ip : "127.0.0.1";
        
        const char* log_port = std::getenv("YIBO_LOG_PORT");
        uint16_t port = log_port ? static_cast<uint16_t>(std::atoi(log_port)) : 9000;
        
        auto param = CSockParam::MakeIPv4(ip, port, SOCK_ISUDP | SOCK_ISIP);
        auto result = g_log_client.Init(param);

        if (result.IsErr()) {
            std::cerr << "[LOG FALLBACK][Init] " << result.Error().ToString() << ": " << info.content << std::endl;
            return;
        }

        // Set receive timeout for ACK waiting (100ms)
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        setsockopt(static_cast<int>(g_log_client), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Prepare server address for sendto
        sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(&g_server_addr);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr->sin_addr);
        g_server_addr_len = sizeof(sockaddr_in);

        g_initialized = true;
    }

    // Serialize log data
    auto serialized = info.Serialize();
    if (serialized.IsErr()) {
        return;
    }

    // Prepare RUDP packet: [Seq(4 bytes)][Data]
    Buffer packet;
    packet.reserve(4 + serialized.Value().size());
    
    // Pack sequence number (big-endian)
    uint32_t seq = g_seq++;
    for (int i = 3; i >= 0; --i) {
        packet.push_back(static_cast<char>((seq >> (i * 8)) & 0xFF));
    }
    packet.append(serialized.Value());

    // Retry logic: up to 3 attempts
    const int MAX_RETRIES = 3;
    bool ack_received = false;

    for (int attempt = 0; attempt < MAX_RETRIES && !ack_received; ++attempt) {
        // Send packet
        auto send_result = g_log_client.SendTo(packet, g_server_addr, g_server_addr_len);
        if (send_result.IsErr()) {
            continue; // Retry
        }

        // Wait for ACK
        Buffer ack_buffer;
        sockaddr_storage from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        auto recv_result = g_log_client.RecvFrom(ack_buffer, 7, &from_addr, &from_len);
        
        if (recv_result.IsOk() && ack_buffer.size() == 7) {
            // Verify ACK format: "ACK" + seq(4 bytes)
            if (ack_buffer[0] == 'A' && ack_buffer[1] == 'C' && ack_buffer[2] == 'K') {
                uint32_t ack_seq = 0;
                for (int i = 0; i < 4; ++i) {
                    ack_seq = (ack_seq << 8) | static_cast<uint8_t>(ack_buffer[3 + i]);
                }
                
                if (ack_seq == seq) {
                    ack_received = true;
                }
            }
        }
    }

    // If all retries failed, log to stderr as fallback
    if (!ack_received) {
        std::cerr << "[LOG FALLBACK][No ACK] " << info.content << std::endl;
    }
}

} // namespace yibo

