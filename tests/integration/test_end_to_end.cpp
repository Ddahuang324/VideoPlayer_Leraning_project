#include <gtest/gtest.h>
#include "server/Server.h"
#include "business/EdoyunPlayerServer.h"
#include "database/Sqlite3Client.h"
#include "network/Socket.h"
#include "network/Epoll.h"
#include "utils/Crypto.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace yibo;

// 端到端集成测试 - 测试完整的客户端-服务器通信
class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试数据库
        auto db = MakeUnique<CSqlite3Client>();
        KeyValue db_params = {{"path", ":memory:"}};

        auto result = db->Connect(db_params);
        ASSERT_TRUE(result.IsOk());

        // 创建用户表
        std::string create_table = R"(
            CREATE TABLE users (
                username TEXT PRIMARY KEY,
                password TEXT NOT NULL
            )
        )";
        result = db->Exec(create_table);
        ASSERT_TRUE(result.IsOk());

        // 插入测试用户
        std::string insert_user = R"(
            INSERT INTO users (username, password) VALUES
            ('e2euser', 'e2epass'),
            ('testclient', 'clientpass')
        )";
        result = db->Exec(insert_user);
        ASSERT_TRUE(result.IsOk());

        m_business = MakeUnique<CEdoyunPlayerServer>(4, std::move(db));
    }

    void TearDown() override {
        m_business.reset();
    }

    UniquePtr<CEdoyunPlayerServer> m_business;
};

// 测试1: Socket创建和基本连接
TEST_F(EndToEndTest, SocketCreationAndBasicSetup) {
    CSocket server_socket;
    CSockParam param = CSockParam::MakeIPv4("127.0.0.1", 0, 0);

    auto result = server_socket.Init(param);
    EXPECT_TRUE(result.IsOk());

    // Socket应该有效
    EXPECT_GT(static_cast<int>(server_socket), 0);
}

// 测试2: Epoll事件处理机制
TEST_F(EndToEndTest, EpollEventHandling) {
    CEpoll epoll;
    auto result = epoll.Create(10);
    ASSERT_TRUE(result.IsOk());

    // 创建测试socket
    CSocket test_socket;
    CSockParam param = CSockParam::MakeIPv4("127.0.0.1", 0, 0);

    result = test_socket.Init(param);
    ASSERT_TRUE(result.IsOk());

    int fd = static_cast<int>(test_socket);

    // 添加到epoll
    EpollData data(fd);
    result = epoll.Add(fd, data, EPOLLIN);
    EXPECT_TRUE(result.IsOk());

    // 修改事件
    result = epoll.Modify(fd, EPOLLOUT, data);
    EXPECT_TRUE(result.IsOk());

    // 删除事件
    result = epoll.Del(fd);
    EXPECT_TRUE(result.IsOk());
}

// 测试3: 模拟完整的客户端请求-服务器响应流程
TEST_F(EndToEndTest, ClientServerRequestResponse) {
    // 创建socketpair用于测试（模拟客户端-服务器通信）
    int sockfds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds);
    ASSERT_EQ(ret, 0);

    int client_fd = sockfds[0];
    int server_fd = sockfds[1];

    // 构造测试请求
    std::string user = "e2euser";
    std::string password = "e2epass";
    std::string time_str = "1234567890";
    std::string salt = "testsalt";
    std::string sign = yibo::Crypto::MD5(user + time_str + password + salt);

    std::string request =
        "GET /api?user=" + user +
        "&time=" + time_str +
        "&salt=" + salt +
        "&sign=" + sign +
        " HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    // 在子线程中模拟客户端发送请求
    std::thread client_thread([client_fd, request]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ssize_t sent = send(client_fd, request.c_str(), request.size(), 0);
        EXPECT_GT(sent, 0);
    });

    // 主线程模拟服务器接收并处理请求
    char buffer[4096] = {0};
    ssize_t received = recv(server_fd, buffer, sizeof(buffer), 0);
    EXPECT_GT(received, 0);

    std::string received_data(buffer, received);

    // 使用业务逻辑处理请求
    class TestServerSocket : public CSocketBase {
    public:
        int m_fd;
        Buffer response_buffer;

        explicit TestServerSocket(int fd) : m_fd(fd) {}

        Result<void, Error> Init(const CSockParam&) override {
            return Result<void, Error>::Ok();
        }

        Result<ssize_t, Error> Send(std::string_view data) override {
            response_buffer.append(data);
            ssize_t sent = send(m_fd, data.data(), data.size(), 0);
            if (sent < 0) {
                return Result<ssize_t, Error>::Err(Error(ErrorCode::SendFailed, "Send failed"));
            }
            return Result<ssize_t, Error>::Ok(sent);
        }

        Result<ssize_t, Error> Recv(Buffer&, size_t) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        Result<ssize_t, Error> SendTo(std::string_view, const sockaddr_storage&, socklen_t) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        Result<ssize_t, Error> RecvFrom(Buffer&, size_t, sockaddr_storage*, socklen_t*) override {
            return Result<ssize_t, Error>::Ok(0);
        }

        void Close() override {}
        operator int() const override { return m_fd; }
    };

    TestServerSocket server_socket(server_fd);
    auto result = m_business->Received(&server_socket, received_data);
    EXPECT_TRUE(result.IsOk());

    // 客户端接收响应
    client_thread.join();

    char response[4096] = {0};
    ssize_t resp_len = recv(client_fd, response, sizeof(response), 0);
    EXPECT_GT(resp_len, 0);

    std::string response_str(response, resp_len);
    EXPECT_NE(response_str.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(response_str.find("application/json"), std::string::npos);

    close(client_fd);
    close(server_fd);
}

// 测试4: 多个客户端并发连接
TEST_F(EndToEndTest, MultipleConcurrentClients) {
    const int num_clients = 5;
    std::vector<std::thread> client_threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back([this, i, &success_count]() {
            // 创建socketpair
            int sockfds[2];
            int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds);
            if (ret != 0) return;

            int client_fd = sockfds[0];
            int server_fd = sockfds[1];

            // 构造请求
            std::string user = (i % 2 == 0) ? "e2euser" : "testclient";
            std::string password = (i % 2 == 0) ? "e2epass" : "clientpass";
            std::string time_str = std::to_string(1234567890 + i);
            std::string salt = "salt" + std::to_string(i);
            std::string sign = yibo::Crypto::MD5(user + time_str + password + salt);

            std::string request =
                "GET /api?user=" + user +
                "&time=" + time_str +
                "&salt=" + salt +
                "&sign=" + sign +
                " HTTP/1.1\r\n\r\n";

            // 发送请求
            send(client_fd, request.c_str(), request.size(), 0);

            // 服务器端接收并处理
            char buffer[4096] = {0};
            ssize_t received = recv(server_fd, buffer, sizeof(buffer), 0);
            if (received > 0) {
                std::string received_data(buffer, received);

                class TestSocket : public CSocketBase {
                public:
                    int m_fd;
                    explicit TestSocket(int fd) : m_fd(fd) {}

                    Result<void, Error> Init(const CSockParam&) override {
                        return Result<void, Error>::Ok();
                    }
                    Result<ssize_t, Error> Send(std::string_view data) override {
                        ssize_t sent = send(m_fd, data.data(), data.size(), 0);
                        return Result<ssize_t, Error>::Ok(sent);
                    }
                    Result<ssize_t, Error> Recv(Buffer&, size_t) override {
                        return Result<ssize_t, Error>::Ok(0);
                    }
                    Result<ssize_t, Error> SendTo(std::string_view, const sockaddr_storage&, socklen_t) override {
                        return Result<ssize_t, Error>::Ok(0);
                    }
                    Result<ssize_t, Error> RecvFrom(Buffer&, size_t, sockaddr_storage*, socklen_t*) override {
                        return Result<ssize_t, Error>::Ok(0);
                    }
                    void Close() override {}
                    operator int() const override { return m_fd; }
                };

                TestSocket server_socket(server_fd);
                auto result = m_business->Received(&server_socket, received_data);

                // 接收响应
                char response[4096] = {0};
                ssize_t resp_len = recv(client_fd, response, sizeof(response), 0);

                if (resp_len > 0 && result.IsOk()) {
                    std::string resp_str(response, resp_len);
                    if (resp_str.find("200 OK") != std::string::npos) {
                        success_count++;
                    }
                }
            }

            close(client_fd);
            close(server_fd);
        });
    }

    for (auto& t : client_threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_clients);
}

// 测试5: 错误处理 - 连接中断
TEST_F(EndToEndTest, ConnectionInterruption) {
    int sockfds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds);
    ASSERT_EQ(ret, 0);

    int client_fd = sockfds[0];
    int server_fd = sockfds[1];

    // 客户端发送不完整的请求后关闭连接
    std::string partial_request = "GET /api?user=test";
    send(client_fd, partial_request.c_str(), partial_request.size(), 0);
    close(client_fd);  // 立即关闭

    // 服务器尝试接收
    char buffer[4096] = {0};
    ssize_t received = recv(server_fd, buffer, sizeof(buffer), 0);

    // 应该能检测到连接关闭
    EXPECT_GE(received, 0);  // 可能是0表示关闭，或者是部分数据

    close(server_fd);
}
