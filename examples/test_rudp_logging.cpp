#include "logger/Logger.h"
#include "logger/LoggerServer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace yibo;

int main() {
    std::cout << "=== RUDP Logging System Test ===" << std::endl;

    // Start the logger server
    std::cout << "Starting LoggerServer on UDP port 9000..." << std::endl;
    auto& server = CLoggerServer::Instance();
    auto result = server.Start("./log");
    
    if (result.IsErr()) {
        std::cerr << "Failed to start logger server: " << result.Error().ToString() << std::endl;
        return 1;
    }
    
    std::cout << "LoggerServer started successfully!" << std::endl;
    
    // Give server time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Send some test logs
    std::cout << "\nSending test logs via RUDP..." << std::endl;
    
    TRACEI("This is an INFO log message");
    TRACED("This is a DEBUG log message with number: %d", 42);
    TRACEW("This is a WARNING log message");
    TRACEE("This is an ERROR log message");
    
    // Test with some data
    const char* test_data = "Hello RUDP!";
    DUMPI(test_data, strlen(test_data), "Dumping test data");
    
    std::cout << "Logs sent! Check ./log directory for output files." << std::endl;
    
    // Give time for logs to be written
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Stop the server
    std::cout << "\nStopping LoggerServer..." << std::endl;
    server.Stop();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}
