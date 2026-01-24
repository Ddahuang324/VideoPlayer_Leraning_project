#include <gtest/gtest.h>
#include "logger/LogInfo.h"
#include "logger/Logger.h"
#include "logger/LoggerServer.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace yibo;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set environment variable for log directory
        setenv("YIBO_LOG_DIR", "./test_log", 1);

        // Clean up any existing log directory
        if (std::filesystem::exists("./test_log")) {
            std::filesystem::remove_all("./test_log");
        }
    }

    void TearDown() override {
        auto& logger = CLoggerServer::Instance();
        (void)logger.Stop();

        // Clean up test log directory
        if (std::filesystem::exists("./test_log")) {
            std::filesystem::remove_all("./test_log");
        }
    }
};

TEST_F(LoggerTest, LogInfoSerialization) {
    LogInfo info(LogLevel::INFO, "test.cpp", 42, "TestFunc", "Test message %d", 123);

    auto serialized = info.Serialize();
    ASSERT_TRUE(serialized.IsOk());

    auto deserialized_res = LogInfo::Deserialize(serialized.Value());
    ASSERT_TRUE(deserialized_res.IsOk());
    auto deserialized = deserialized_res.Value().first;

    EXPECT_EQ(deserialized.level, LogLevel::INFO);
    EXPECT_EQ(deserialized.file, "test.cpp");
    EXPECT_EQ(deserialized.line, 42);
    EXPECT_EQ(deserialized.function, "TestFunc");
    EXPECT_EQ(deserialized.content, "Test message 123");
}

TEST_F(LoggerTest, LogInfoWithDump) {
    LogInfo info(LogLevel::INFO, "test.cpp", 42, "TestFunc", "Test with dump");
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    info.dump_data = Optional<Buffer>(Buffer((const char*)data, sizeof(data)));

    auto serialized = info.Serialize();
    ASSERT_TRUE(serialized.IsOk());

    auto deserialized_res = LogInfo::Deserialize(serialized.Value());
    ASSERT_TRUE(deserialized_res.IsOk());
    auto deserialized = deserialized_res.Value().first;

    EXPECT_TRUE(deserialized.dump_data.HasValue());
    EXPECT_EQ(deserialized.dump_data.Value().size(), 4);
}

TEST_F(LoggerTest, ServerStartStop) {
    auto& logger = CLoggerServer::Instance();

    auto result = logger.Start("./test_log");
    ASSERT_TRUE(result.IsOk());

    // Give server time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify log directory was created
    EXPECT_TRUE(std::filesystem::exists("./test_log"));

    result = logger.Stop();
    EXPECT_TRUE(result.IsOk());
}

TEST_F(LoggerTest, BasicLogging) {
    auto& logger = CLoggerServer::Instance();
    auto result = logger.Start("./test_log");
    ASSERT_TRUE(result.IsOk());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send some logs
    TRACEI("Test info message %d", 1);
    TRACED("Test debug message %d", 2);
    TRACEW("Test warning message %d", 3);
    TRACEE("Test error message %d", 4);

    // Give time for logs to be written
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop logger to ensure everything is flushed and file is closed
    (void)logger.Stop();

    // Check that log file was created
    bool log_file_exists = false;
    for (const auto& entry : std::filesystem::directory_iterator("./test_log")) {
        if (entry.path().extension() == ".log") {
            log_file_exists = true;

            // Read log file and verify content
            std::ifstream file(entry.path());
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

            EXPECT_TRUE(content.find("Test info message 1") != std::string::npos);
            EXPECT_TRUE(content.find("Test debug message 2") != std::string::npos);
            EXPECT_TRUE(content.find("Test warning message 3") != std::string::npos);
            EXPECT_TRUE(content.find("Test error message 4") != std::string::npos);
            break;
        }
    }

    EXPECT_TRUE(log_file_exists);
}

TEST_F(LoggerTest, LevelToString) {
    EXPECT_STREQ(LevelToString(LogLevel::INFO), "INFO ");
    EXPECT_STREQ(LevelToString(LogLevel::DEBUG), "DEBUG");
    EXPECT_STREQ(LevelToString(LogLevel::WARNING), "WARN ");
    EXPECT_STREQ(LevelToString(LogLevel::ERROR), "ERROR");
    EXPECT_STREQ(LevelToString(LogLevel::FATAL), "FATAL");
}
