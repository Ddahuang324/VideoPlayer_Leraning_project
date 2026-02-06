#include <iostream>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include "logger/LoggerServer.h"
#include "common/ServerConfig.h"

using namespace yibo;

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n🛑 Received shutdown signal..." << std::endl;
        g_running = false;
    }
}

void PrintUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -c, --config <file>    Specify configuration file (default: config/server.json)\n"
              << "  -d, --log-dir <dir>    Specify log directory (overrides config)\n"
              << "  -p, --port <port>      Specify UDP port (default: 9000)\n"
              << "  -h, --help             Show this help message\n"
              << "  -v, --version          Show version information\n"
              << "\nExamples:\n"
              << "  " << program_name << "                          # Use default config\n"
              << "  " << program_name << " -d /var/log/yiboserver  # Custom log directory\n"
              << "  " << program_name << " -p 9001                 # Custom UDP port\n"
              << std::endl;
}

void PrintBanner() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                       ║\n";
    std::cout << "║          📝 YiboServer Logger Service v1.0.0          ║\n";
    std::cout << "║         High-Performance Logging System               ║\n";
    std::cout << "║                                                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Default values
    std::string config_path = "config/server.json";
    std::string log_dir;
    std::string port_str;
    bool use_config = true;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            std::cout << "YiboServer Logger Service v1.0.0" << std::endl;
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_path = argv[++i];
            } else {
                std::cerr << "Error: --config requires an argument\n";
                PrintUsage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--log-dir") == 0) {
            if (i + 1 < argc) {
                log_dir = argv[++i];
                use_config = false;
            } else {
                std::cerr << "Error: --log-dir requires an argument\n";
                PrintUsage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port_str = argv[++i];
            } else {
                std::cerr << "Error: --port requires an argument\n";
                PrintUsage(argv[0]);
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // Print banner
    PrintBanner();

    // Load configuration if needed
    if (use_config && log_dir.empty()) {
        std::cout << "📂 Loading configuration from: " << config_path << std::endl;
        auto config_result = ServerConfig::Load(config_path);
        
        if (config_result.IsOk()) {
            auto config = config_result.Value();
            log_dir = config.GetLogOutputDir();
            std::cout << "✅ Configuration loaded successfully" << std::endl;
        } else {
            std::cout << "⚠️  Failed to load config, using default: ./logs" << std::endl;
            log_dir = "./logs";
        }
    }

    // Use default if still empty
    if (log_dir.empty()) {
        log_dir = "./logs";
    }

    // Set environment variable for port if specified
    if (!port_str.empty()) {
        setenv("YIBO_LOG_PORT", port_str.c_str(), 1);
    }

    // Get port for display
    const char* env_port = std::getenv("YIBO_LOG_PORT");
    uint16_t port = env_port ? static_cast<uint16_t>(std::atoi(env_port)) : 9000;

    // Display configuration
    std::cout << "📋 Logger Service Configuration:\n";
    std::cout << "  ├─ Log Directory: " << log_dir << "\n";
    std::cout << "  ├─ UDP Port:      " << port << "\n";
    std::cout << "  └─ Protocol:      RUDP (Reliable UDP)\n";
    std::cout << "\n";

    // Start logger server
    std::cout << "🚀 Starting logger service..." << std::endl;
    auto& logger_server = CLoggerServer::Instance();
    auto result = logger_server.Start(log_dir);

    if (result.IsErr()) {
        std::cerr << "❌ Failed to start logger service: " 
                  << result.Error().message << std::endl;
        return 1;
    }

    std::cout << "✅ Logger service started successfully!" << std::endl;
    std::cout << "\n";
    std::cout << "📡 Listening for log messages on UDP port " << port << "\n";
    std::cout << "📁 Writing logs to: " << log_dir << "\n";
    std::cout << "\n";
    std::cout << "💡 Applications can now send logs using:\n";
    std::cout << "   TRACEI(\"message\"), TRACED(...), TRACEW(...), etc.\n";
    std::cout << "\n";
    std::cout << "📝 Press Ctrl+C to stop the logger service\n";
    std::cout << "\n";

    // Keep running until shutdown signal
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Graceful shutdown
    std::cout << "\n🛑 Shutting down logger service..." << std::endl;
    
    auto stop_result = logger_server.Stop();
    if (stop_result.IsOk()) {
        std::cout << "✅ Logger service stopped gracefully" << std::endl;
    } else {
        std::cerr << "⚠️  Logger service stopped with warnings" << std::endl;
    }

    return 0;
}
