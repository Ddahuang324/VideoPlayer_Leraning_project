#include "logger/Logger.h"
#include "network/Socket.h"
#include <iostream>

namespace yibo {

void Trace(const LogInfo& info) {
    static thread_local CSocket g_log_client;
    static thread_local bool g_connected = false;

    // Lazy initialization and connection
    if (!g_connected) {
        const char* log_dir = std::getenv("YIBO_LOG_DIR");
        std::string socket_path = log_dir ? std::string(log_dir) + "/server.sock" : "./log/server.sock";
        auto param = CSockParam::MakeUnix(socket_path, 0);
        auto result = g_log_client.Init(param);

        if (result.IsErr()) {
            std::cerr << "[LOG FALLBACK][Init] " << result.Error().ToString() << ": " << info.content << std::endl;
            return;
        }

        result = g_log_client.Link();
        if (result.IsErr()) {
            std::cerr << "[LOG FALLBACK][Link] " << result.Error().ToString() << ": " << info.content << std::endl;
            return;
        }

        g_connected = true;
    }

    // Serialize and send
    auto serialized = info.Serialize();
    if (serialized.IsErr()) {
        return;
    }

    auto send_result = g_log_client.Send(serialized.Value());
    if (send_result.IsErr()) {
        g_connected = false;  // Mark as disconnected to retry next time
    }
}

} // namespace yibo
