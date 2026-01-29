#include <gtest/gtest.h>
#include "network/Socket.h"
#include <thread>
#include <chrono>

using namespace yibo;

TEST(CSockParamTest, MakeIPv4) {
    auto param = CSockParam::MakeIPv4("127.0.0.1", 8080, SOCK_ISSERVER);

    EXPECT_TRUE(param.IsServer());
    EXPECT_TRUE(param.IsIP());
    EXPECT_FALSE(param.IsNonBlock());
    EXPECT_FALSE(param.IsUDP());
    EXPECT_NE(param.GetAddr(), nullptr);
    EXPECT_GT(param.GetAddrLen(), 0);
}

TEST(CSockParamTest, MakeUnix) {
    auto param = CSockParam::MakeUnix("/tmp/test.sock", 0);

    EXPECT_FALSE(param.IsServer());
    EXPECT_FALSE(param.IsIP());
    EXPECT_NE(param.GetAddr(), nullptr);
    EXPECT_GT(param.GetAddrLen(), 0);
}

TEST(CSockParamTest, InvalidIPv4) {
    auto param = CSockParam::MakeIPv4("invalid", 8080, 0);
    EXPECT_EQ(param.GetAddr(), nullptr);
}

TEST(CSocketTest, InitAndClose) {
    auto socket = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("127.0.0.1", 0, 0);

    ASSERT_TRUE(socket->Init(param).IsOk());
    EXPECT_NE(static_cast<int>(*socket), -1);

    socket->Close();
    EXPECT_EQ(static_cast<int>(*socket), -1);
}

TEST(CSocketTest, DoubleInit) {
    auto socket = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("127.0.0.1", 0, 0);

    ASSERT_TRUE(socket->Init(param).IsOk());

    auto result = socket->Init(param);
    ASSERT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::AlreadyInitialized);
}

TEST(CSocketTest, MoveConstructor) {
    auto socket1 = MakeUnique<CSocket>();
    socket1->Init(CSockParam::MakeIPv4("127.0.0.1", 0, 0));
    int fd = static_cast<int>(*socket1);

    auto socket2 = MakeUnique<CSocket>(std::move(*socket1));
    EXPECT_EQ(static_cast<int>(*socket2), fd);
    EXPECT_EQ(static_cast<int>(*socket1), -1);
}

TEST(CSocketTest, MoveAssignment) {
    auto socket1 = MakeUnique<CSocket>();
    socket1->Init(CSockParam::MakeIPv4("127.0.0.1", 0, 0));
    int fd = static_cast<int>(*socket1);

    CSocket socket2;
    socket2 = std::move(*socket1);
    EXPECT_EQ(static_cast<int>(socket2), fd);
    EXPECT_EQ(static_cast<int>(*socket1), -1);
}

TEST(CSocketTest, SendWithoutInit) {
    auto socket = MakeUnique<CSocket>();

    auto result = socket->Send("test");
    ASSERT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::NotInitialized);
}

TEST(CSocketTest, RecvWithoutInit) {
    auto socket = MakeUnique<CSocket>();
    Buffer buf;

    auto result = socket->Recv(buf, 1024);
    ASSERT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::NotInitialized);
}

TEST(CSocketTest, ServerBindAndListen) {
    auto server = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("127.0.0.1", 0, SOCK_ISSERVER | SOCK_ISREUSE);

    ASSERT_TRUE(server->Init(param).IsOk());
    ASSERT_TRUE(server->Link().IsOk());
}

TEST(CSocketIntegrationTest, EchoServerClient) {
    auto server = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("127.0.0.1", 0, SOCK_ISSERVER | SOCK_ISREUSE);

    ASSERT_TRUE(server->Init(param).IsOk());
    ASSERT_TRUE(server->Link().IsOk());

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(static_cast<int>(*server), reinterpret_cast<sockaddr*>(&addr), &len);
    uint16_t port = ntohs(addr.sin_port);

    std::thread server_thread([&server]() {
        auto accept_result = server->Accept();
        if (accept_result.IsOk()) {
            auto conn = std::move(accept_result.Value());
            Buffer buf;
            auto recv_result = conn->Recv(buf, 1024);
            if (recv_result.IsOk()) {
                conn->Send(buf);
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto client = MakeUnique<CSocket>();
    auto client_param = CSockParam::MakeIPv4("127.0.0.1", port, 0);
    ASSERT_TRUE(client->Init(client_param).IsOk());
    ASSERT_TRUE(client->Link().IsOk());

    ASSERT_TRUE(client->Send("Hello").IsOk());

    Buffer buf;
    auto recv_result = client->Recv(buf, 1024);
    ASSERT_TRUE(recv_result.IsOk());
    EXPECT_EQ(buf, "Hello");

    server_thread.join();
}

TEST(CSocketTest, NonBlockingMode) {
    auto socket = MakeUnique<CSocket>();
    auto param = CSockParam::MakeIPv4("127.0.0.1", 0, SOCK_ISSERVER | SOCK_ISNONBLOCK);

    ASSERT_TRUE(socket->Init(param).IsOk());
    ASSERT_TRUE(socket->Link().IsOk());

    auto result = socket->Accept();
    ASSERT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::WouldBlock);
}
