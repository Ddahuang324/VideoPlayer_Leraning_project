#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "business/EdoyunPlayerServer.h"
#include "database/DatabaseClient.h"
#include "network/Socket.h"
#include "utils/Crypto.h"

using namespace yibo;
using namespace testing;

class MockDatabaseClient : public CDatabaseClient {
public:
    MOCK_METHOD((Result<void, Error>), Connect, (const KeyValue& args), (override));
    MOCK_METHOD((Result<void, Error>), Exec, (const std::string& sql), (override));
    MOCK_METHOD((Result<void, Error>), Exec, (const std::string& sql, Table& table), (override));
    MOCK_METHOD((Result<void, Error>), StartTransaction, (), (override));
    MOCK_METHOD((Result<void, Error>), CommitTransaction, (), (override));
    MOCK_METHOD((Result<void, Error>), RollbackTransaction, (), (override));
    MOCK_METHOD((Result<void, Error>), Close, (), (override));
    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(std::string, Escape, (const std::string& str), (override));
};

class MockSocket : public CSocketBase {
public:
    MOCK_METHOD((Result<void, Error>), Init, (const CSockParam& param), (override));
    MOCK_METHOD((Result<ssize_t, Error>), Send, (std::string_view data), (override));
    MOCK_METHOD((Result<ssize_t, Error>), Recv, (Buffer& buffer, size_t max_size), (override));
    MOCK_METHOD((Result<ssize_t, Error>), SendTo, (std::string_view data, const sockaddr_storage& addr, socklen_t addr_len), (override));
    MOCK_METHOD((Result<ssize_t, Error>), RecvFrom, (Buffer& buffer, size_t max_size, sockaddr_storage* addr, socklen_t* addr_len), (override));
    MOCK_METHOD(void, Close, (), (override));
    operator int() const override { return 0; }
};

TEST(EdoyunPlayerServerTest, HandleValidRequest) {
    auto mock_db = MakeUnique<MockDatabaseClient>();
    auto* db_ptr = mock_db.get();
    
    CEdoyunPlayerServer server(4, std::move(mock_db));
    MockSocket mock_socket;

    std::string user = "testuser";
    std::string password = "testpassword";
    std::string time_str = "123456789";
    std::string salt = "random_salt";
    std::string sign = Crypto::MD5(user + time_str + password + salt);

    std::string request = "GET /api?user=" + user + "&time=" + time_str + "&salt=" + salt + "&sign=" + sign + " HTTP/1.1\r\n\r\n";

    // Expect DB Escape
    EXPECT_CALL(*db_ptr, Escape(user))
        .WillOnce(Return(user));

    // Expect DB Exec to fetch password
    EXPECT_CALL(*db_ptr, Exec(_, _))
        .WillOnce(Invoke([&](const std::string& sql, Table& table) {
            table.GetFields()[0].SetValue(user);
            table.GetFields()[1].SetValue(password);
            return Result<void, Error>::Ok();
        }));

    // Expect Socket Send success message
    EXPECT_CALL(mock_socket, Send(ContainsRegex("HTTP/1.1 200 OK")))
        .WillOnce(Return(Result<ssize_t, Error>::Ok(0)));

    auto result = server.Received(&mock_socket, request);
    EXPECT_TRUE(result.IsOk());
}

TEST(EdoyunPlayerServerTest, HandleInvalidSignature) {
    auto mock_db = MakeUnique<MockDatabaseClient>();
    auto* db_ptr = mock_db.get();
    
    CEdoyunPlayerServer server(4, std::move(mock_db));
    MockSocket mock_socket;

    std::string user = "testuser";
    std::string password = "testpassword";
    std::string time_str = "123456789";
    std::string salt = "random_salt";
    std::string sign = "wrong_sign";

    std::string request = "GET /api?user=" + user + "&time=" + time_str + "&salt=" + salt + "&sign=" + sign + " HTTP/1.1\r\n\r\n";

    EXPECT_CALL(*db_ptr, Escape(user))
        .WillOnce(Return(user));

    EXPECT_CALL(*db_ptr, Exec(_, _))
        .WillOnce(Invoke([&](const std::string& sql, Table& table) {
            table.GetFields()[0].SetValue(user);
            table.GetFields()[1].SetValue(password);
            return Result<void, Error>::Ok();
        }));

    // Expect 403 Forbidden
    EXPECT_CALL(mock_socket, Send(ContainsRegex("HTTP/1.1 403")))
        .WillOnce(Return(Result<ssize_t, Error>::Ok(0)));

    auto result = server.Received(&mock_socket, request);
    EXPECT_FALSE(result.IsOk());
}

TEST(EdoyunPlayerServerTest, HandleUserNotFound) {
    auto mock_db = MakeUnique<MockDatabaseClient>();
    auto* db_ptr = mock_db.get();
    
    CEdoyunPlayerServer server(4, std::move(mock_db));
    MockSocket mock_socket;

    std::string request = "GET /api?user=nonexistent&time=1&salt=2&sign=3 HTTP/1.1\r\n\r\n";

    EXPECT_CALL(*db_ptr, Escape("nonexistent"))
        .WillOnce(Return("nonexistent"));

    EXPECT_CALL(*db_ptr, Exec(_, _))
        .WillOnce(Invoke([&](const std::string& sql, Table& table) {
            // Keep fields Null
            return Result<void, Error>::Ok();
        }));

    // Expect 404 Not Found
    EXPECT_CALL(mock_socket, Send(ContainsRegex("HTTP/1.1 404")))
        .WillOnce(Return(Result<ssize_t, Error>::Ok(0)));

    auto result = server.Received(&mock_socket, request);
    EXPECT_FALSE(result.IsOk());
}
