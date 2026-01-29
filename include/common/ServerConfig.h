#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include "common/Result.h"
#include "common/Error.h"

namespace yibo {

using json = nlohmann::json;

/**
 * @brief Server configuration manager
 * 
 * Loads and manages server configuration from JSON files.
 * Provides type-safe access to configuration values.
 */
class ServerConfig {
public:
    /**
     * @brief Load configuration from file
     * @param config_path Path to JSON configuration file
     * @return Result with success or error
     */
    static Result<ServerConfig, Error> Load(const std::string& config_path);

    /**
     * @brief Load default configuration
     * @return ServerConfig with default values
     */
    static ServerConfig Default();

    // Server settings
    std::string GetServerName() const { return m_config["server"]["name"]; }
    std::string GetServerVersion() const { return m_config["server"]["version"]; }
    std::string GetHost() const { return m_config["server"]["host"]; }
    int GetPort() const { return m_config["server"]["port"]; }
    int GetMaxConnections() const { return m_config["server"]["max_connections"]; }
    int GetTimeoutSeconds() const { return m_config["server"]["timeout_seconds"]; }
    int GetWorkerThreads() const { return m_config["server"]["worker_threads"]; }

    // Database settings
    std::string GetDatabaseType() const { return m_config["database"]["type"]; }
    std::string GetSqlitePath() const { return m_config["database"]["sqlite"]["path"]; }
    std::string GetMysqlHost() const { return m_config["database"]["mysql"]["host"]; }
    int GetMysqlPort() const { return m_config["database"]["mysql"]["port"]; }
    std::string GetMysqlUser() const { return m_config["database"]["mysql"]["user"]; }
    std::string GetMysqlPassword() const { return m_config["database"]["mysql"]["password"]; }
    std::string GetMysqlDatabase() const { return m_config["database"]["mysql"]["database"]; }

    // Logger settings
    std::string GetLogLevel() const { return m_config["logger"]["level"]; }
    std::string GetLogOutputDir() const { return m_config["logger"]["output_dir"]; }
    std::string GetLogFilePrefix() const { return m_config["logger"]["file_prefix"]; }
    int GetLogMaxFileSizeMB() const { return m_config["logger"]["max_file_size_mb"]; }
    int GetLogMaxFiles() const { return m_config["logger"]["max_files"]; }
    bool GetLogAsyncMode() const { return m_config["logger"]["async_mode"]; }
    bool GetLogConsoleOutput() const { return m_config["logger"]["console_output"]; }

    // Security settings
    bool GetEnableSignature() const { return m_config["security"]["enable_signature"]; }
    int GetSignatureTimeoutSeconds() const { return m_config["security"]["signature_timeout_seconds"]; }
    bool GetEnableRateLimit() const { return m_config["security"]["enable_rate_limit"]; }

    // Performance settings
    int GetEpollMaxEvents() const { return m_config["performance"]["epoll_max_events"]; }
    bool GetEnableTcpNodelay() const { return m_config["performance"]["enable_tcp_nodelay"]; }

    // Get raw JSON for custom access
    const json& GetRawConfig() const { return m_config; }

    // Print configuration (for debugging)
    std::string ToString() const { return m_config.dump(2); }

private:
    ServerConfig() = default;
    explicit ServerConfig(json config) : m_config(std::move(config)) {}

    json m_config;
};

} // namespace yibo
