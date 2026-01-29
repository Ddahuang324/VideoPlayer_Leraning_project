#include "common/ServerConfig.h"
#include <iostream>

namespace yibo {

Result<ServerConfig, Error> ServerConfig::Load(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            return Result<ServerConfig, Error>::Err(
                Error(ErrorCode::FileNotFound, 
                      "Failed to open config file: " + config_path));
        }

        json config;
        file >> config;

        // Validate required fields
        if (!config.contains("server") || !config.contains("database") || 
            !config.contains("logger")) {
            return Result<ServerConfig, Error>::Err(
                Error(ErrorCode::InvalidArgument, 
                      "Invalid config: missing required sections"));
        }

        return Result<ServerConfig, Error>::Ok(ServerConfig(config));

    } catch (const json::exception& e) {
        return Result<ServerConfig, Error>::Err(
            Error(ErrorCode::InvalidArgument, 
                  std::string("JSON parse error: ") + e.what()));
    } catch (const std::exception& e) {
        return Result<ServerConfig, Error>::Err(
            Error(ErrorCode::Unknown, 
                  std::string("Config load error: ") + e.what()));
    }
}

ServerConfig ServerConfig::Default() {
    json default_config = {
        {"server", {
            {"name", "YiboServer"},
            {"version", "1.0.0"},
            {"host", "0.0.0.0"},
            {"port", 8080},
            {"max_connections", 1000},
            {"timeout_seconds", 30},
            {"worker_threads", 4}
        }},
        {"database", {
            {"type", "sqlite3"},
            {"sqlite", {
                {"path", "./data/yiboserver.db"}
            }}
        }},
        {"logger", {
            {"level", "INFO"},
            {"output_dir", "./logs"},
            {"file_prefix", "yiboserver"},
            {"max_file_size_mb", 100},
            {"max_files", 10},
            {"async_mode", true},
            {"console_output", true}
        }},
        {"security", {
            {"enable_signature", true},
            {"signature_timeout_seconds", 300},
            {"enable_rate_limit", true}
        }},
        {"performance", {
            {"epoll_max_events", 1024},
            {"enable_tcp_nodelay", true}
        }}
    };

    return ServerConfig(default_config);
}

} // namespace yibo
