#include "logger/Logger.h"
#include "logger/LoggerServer.h"
#include <thread>
#include <chrono>

int main() {
    // Start the logger server
    auto& logger = yibo::CLoggerServer::Instance();
    auto result = logger.Start("./log");

    if (result.IsErr()) {
        std::cerr << "Failed to start logger: " << result.Error().Message() << std::endl;
        return 1;
    }

    // Give the server time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test different log levels
    TRACEI("Server started successfully on port %d", 8080);
    TRACED("Debug information: connection count = %d", 42);
    TRACEW("Warning: high memory usage detected");
    TRACEE("Error: failed to connect to database");

    // Test memory dump
    uint8_t buffer[16] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
                          0x72, 0x6C, 0x64, 0x21, 0x00, 0x01, 0x02, 0x03};
    DUMPI(buffer, sizeof(buffer), "Received packet from client");

    // Give time for logs to be written
    std::this_thread::sleep_for(std::chrono::seconds(1));

    logger.Stop();

    return 0;
}
