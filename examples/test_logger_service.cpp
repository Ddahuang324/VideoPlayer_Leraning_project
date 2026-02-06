#include "logger/Logger.h"
#include <thread>
#include <chrono>

int main() {
    std::cout << "Testing logger connection..." << std::endl;
    
    // Test different log levels
    TRACEI("Test INFO message: Logger service is working!");
    TRACED("Test DEBUG message: Connection established");
    TRACEW("Test WARNING message: This is a test warning");
    TRACEE("Test ERROR message: This is a test error");
    
    // Test with formatted output
    TRACEI("Server started on port %d with %d worker threads", 8080, 4);
    
    // Test binary dump
    uint8_t buffer[16] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
                          0x72, 0x6C, 0x64, 0x21, 0x00, 0x01, 0x02, 0x03};
    DUMPI(buffer, sizeof(buffer), "Test packet dump");
    
    std::cout << "Logs sent! Check logs/ directory for output." << std::endl;
    std::cout << "Waiting 1 second for logs to be written..." << std::endl;
    
    // Give time for logs to be written
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "Done!" << std::endl;
    
    return 0;
}
