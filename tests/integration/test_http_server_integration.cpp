#include <gtest/gtest.h>
#include "business/EdoyunPlayerServer.h"
#include "database/Sqlite3Client.h"
#include "database/MysqlClient.h"
#include "network/Socket.h"
#include "utils/Crypto.h"
#include "http/CHttpParser.h"
#include <thread>
#include <chrono>

using namespace yibo;

// HTTP服务器全链路集成测试
class HttpServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时SQLite数据库用于测试
        auto db = MakeUnique<CSqlite3Client>();
        KeyValue db_params = {{"path", ":memory:"}};

        auto result = db->Connect(db_params);
        ASSERT_TRUE(result.IsOk()) << "Failed to connect to test database";

        // 创建测试用户表
        std::string create_table = R"(
            CREATE TABLE users (
                username TEXT PRIMARY KEY,
                password TEXT NOT NULL
            )
        )";
        result = db->Exec(create_table);
        ASSERT_TRUE(result.IsOk()) << "Failed to create users table";

        // 插入测试用户
        std::string insert_user = R"(
            INSERT INTO users (username, password) VALUES
            ('testuser', 'testpass123'),
            ('admin', 'adminpass456')
        )";
        result = db->Exec(insert_user);
        ASSERT_TRUE(result.IsOk()) << "Failed to insert test users";

        m_db_ptr = db.get();
        m_server = MakeUnique<CEdoyunPlayerServer>(4, std::move(db));
    }

    void TearDown() override {
        m_server.reset();
    }

    CSqlite3Client* m_db_ptr;
    UniquePtr<CEdoyunPlayerServer> m_server;
};

// 测试1: 完整的HTTP请求处理流程（有效签名）
TEST_F(HttpServerIntegrationTest, ValidHttpRequestFullFlow) {
    std::string user = "testuser";
    std::string password = "testpass123";
    std::string time_str = "1234567890";
    std::string salt = "random_salt_xyz";

    // 计算正确的签名
    std::string sign = Crypto::MD5(user + time_str + password + salt);

    // 构造HTTP GET请求
    std::string request =
        "GET /api?user=" + user +
        "&time=" + time_str +
        "&salt=" + salt +
        "&sign=" + sign +
        " HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: IntegrationTest/1.0\r\n"
        "\r\n";

    // 模拟Socket（用于接收响应）
    class TestSocket : public CSocketBase {
    public:
        Buffer response_buffer;

        Result<void, Error> Init(const CSockParam& param) override {
            return Result<void, Error>::Ok();
        }

        Result<ssize_t, Error> Send(std::string_view data) override {
            response_buffer.append(data);
            return Result<ssize_t, Error>::Ok(data.size());
        }

        Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        Result<ssize_t, Error> SendTo(std::string_view data, const sockaddr_storage& addr, socklen_t addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        Result<ssize_t, Error> RecvFrom(Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        void Close() override {}
        operator int() const override { return 1; }
    };

    TestSocket test_socket;

    // 执行请求处理
    auto result = m_server->Received(&test_socket, request);

    // 验证结果
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(test_socket.response_buffer.empty());
    EXPECT_NE(test_socket.response_buffer.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(test_socket.response_buffer.find("application/json"), std::string::npos);
}

// 测试2: 无效签名的请求
TEST_F(HttpServerIntegrationTest, InvalidSignatureRequest) {
    std::string user = "testuser";
    std::string time_str = "1234567890";
    std::string salt = "random_salt";
    std::string sign = "invalid_signature_hash";

    std::string request =
        "GET /api?user=" + user +
        "&time=" + time_str +
        "&salt=" + salt +
        "&sign=" + sign +
        " HTTP/1.1\r\n\r\n";

    class TestSocket : public CSocketBase {
    public:
        Buffer response_buffer;
        Result<void, Error> Init(const CSockParam& param) override {
            return Result<void, Error>::Ok();
        }
        Result<ssize_t, Error> Send(std::string_view data) override {
            response_buffer.append(data);
            return Result<ssize_t, Error>::Ok(data.size());
        }
        Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> SendTo(std::string_view data, const sockaddr_storage& addr, socklen_t addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> RecvFrom(Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        void Close() override {}
        operator int() const override { return 1; }
    };

    TestSocket test_socket;
    auto result = m_server->Received(&test_socket, request);

    EXPECT_FALSE(result.IsOk());
    EXPECT_NE(test_socket.response_buffer.find("403"), std::string::npos);
}

// 测试3: 用户不存在
TEST_F(HttpServerIntegrationTest, UserNotFound) {
    std::string user = "nonexistent_user";
    std::string time_str = "1234567890";
    std::string salt = "salt";
    std::string sign = "any_sign";

    std::string request =
        "GET /api?user=" + user +
        "&time=" + time_str +
        "&salt=" + salt +
        "&sign=" + sign +
        " HTTP/1.1\r\n\r\n";

    class TestSocket : public CSocketBase {
    public:
        Buffer response_buffer;
        Result<void, Error> Init(const CSockParam& param) override {
            return Result<void, Error>::Ok();
        }
        Result<ssize_t, Error> Send(std::string_view data) override {
            response_buffer.append(data);
            return Result<ssize_t, Error>::Ok(data.size());
        }
        Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> SendTo(std::string_view data, const sockaddr_storage& addr, socklen_t addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> RecvFrom(Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        void Close() override {}
        operator int() const override { return 1; }
    };

    TestSocket test_socket;
    auto result = m_server->Received(&test_socket, request);

    EXPECT_FALSE(result.IsOk());
    EXPECT_NE(test_socket.response_buffer.find("404"), std::string::npos);
}

// 测试4: 测试多个并发请求处理
TEST_F(HttpServerIntegrationTest, ConcurrentRequests) {
    const int num_requests = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_requests; ++i) {
        threads.emplace_back([this, &success_count]() {
            std::string user = "testuser";
            std::string password = "testpass123";
            std::string time_str = std::to_string(std::time(nullptr));
            std::string salt = "salt_" + std::to_string(std::rand());
            std::string sign = Crypto::MD5(user + time_str + password + salt);

            std::string request =
                "GET /api?user=" + user +
                "&time=" + time_str +
                "&salt=" + salt +
                "&sign=" + sign +
                " HTTP/1.1\r\n\r\n";

            class TestSocket : public CSocketBase {
            public:
                Buffer response_buffer;
                Result<void, Error> Init(const CSockParam& param) override {
                    return Result<void, Error>::Ok();
                }
                Result<ssize_t, Error> Send(std::string_view data) override {
                    response_buffer.append(data);
                    return Result<ssize_t, Error>::Ok(data.size());
                }
                Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override {
                    return Result<ssize_t, Error>::Ok(0);
                }
                Result<ssize_t, Error> SendTo(std::string_view data, const sockaddr_storage& addr, socklen_t addr_len) override {
                    return Result<ssize_t, Error>::Ok(0);
                }
                Result<ssize_t, Error> RecvFrom(Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len) override {
                    return Result<ssize_t, Error>::Ok(0);
                }
                void Close() override {}
                operator int() const override { return 1; }
            };

            TestSocket test_socket;
            auto result = m_server->Received(&test_socket, request);

            if (result.IsOk() && test_socket.response_buffer.find("200 OK") != std::string::npos) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_requests);
}

// 测试5: 测试HTTP解析器对各种格式的请求
TEST_F(HttpServerIntegrationTest, VariousHttpFormats) {
    // POST请求
    std::string post_request =
        "POST /api HTTP/1.1\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 50\r\n"
        "\r\n"
        "user=testuser&time=123&salt=abc&sign=xyz";

    class TestSocket : public CSocketBase {
    public:
        Buffer response_buffer;
        Result<void, Error> Init(const CSockParam& param) override {
            return Result<void, Error>::Ok();
        }
        Result<ssize_t, Error> Send(std::string_view data) override {
            response_buffer.append(data);
            return Result<ssize_t, Error>::Ok(data.size());
        }
        Result<ssize_t, Error> Recv(Buffer& buffer, size_t max_size) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> SendTo(std::string_view data, const sockaddr_storage& addr, socklen_t addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        Result<ssize_t, Error> RecvFrom(Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len) override {
            return Result<ssize_t, Error>::Ok(0);
        }
        void Close() override {}
        operator int() const override { return 1; }
    };

    TestSocket test_socket;
    auto result = m_server->Received(&test_socket, post_request);

    // 应该返回响应（即使可能失败也应该有响应）
    EXPECT_FALSE(test_socket.response_buffer.empty());
}
