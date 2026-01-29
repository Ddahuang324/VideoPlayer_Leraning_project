#include <gtest/gtest.h>
#include "database/Sqlite3Client.h"
#include "database/MysqlClient.h"
#include "database/Sqlite3Table.h"
#include "database/Field.h"
#include "concurrent/ThreadPool.h"
#include <thread>
#include <atomic>
#include <vector>

using namespace yibo;

// 测试用表定义
class TestUsersTable : public Sqlite3Table {
public:
    TestUsersTable() {
        m_fields = {
            Field("id", SqlType::TYPE_INT, PRIMARY_KEY | AUTOINCREMENT),
            Field("username", SqlType::TYPE_VARCHAR, NOT_NULL),
            Field("email", SqlType::TYPE_VARCHAR, NOT_NULL),
            Field("balance", SqlType::TYPE_REAL),
            Field("created_at", SqlType::TYPE_INT)
        };
    }
    std::string GetTableName() const override { return "test_users"; }
};

// 数据库并发集成测试
class DatabaseIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_db = MakeUnique<CSqlite3Client>();
        KeyValue params = {{"path", ":memory:"}};

        auto result = m_db->Connect(params);
        ASSERT_TRUE(result.IsOk());

        // 创建测试表
        std::string create_sql = R"(
            CREATE TABLE test_users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL,
                email TEXT NOT NULL,
                balance REAL DEFAULT 0.0,
                created_at INTEGER
            )
        )";

        result = m_db->Exec(create_sql);
        ASSERT_TRUE(result.IsOk());
    }

    void TearDown() override {
        if (m_db) {
            auto result = m_db->Close();
            (void)result;  // 忽略返回值警告
        }
    }

    UniquePtr<CSqlite3Client> m_db;
};

// 测试1: 基本的CRUD操作
TEST_F(DatabaseIntegrationTest, BasicCRUDOperations) {
    // Create
    std::string insert_sql = R"(
        INSERT INTO test_users (username, email, balance, created_at)
        VALUES ('alice', 'alice@example.com', 100.5, 1234567890)
    )";
    auto result = m_db->Exec(insert_sql);
    EXPECT_TRUE(result.IsOk());

    // Read
    std::string select_sql = "SELECT * FROM test_users WHERE username='alice'";
    TestUsersTable table;

    result = m_db->Exec(select_sql, table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(table.GetFields()[0].IsNull()); // id
    EXPECT_FALSE(table.GetFields()[1].IsNull()); // username
    EXPECT_EQ(std::get<std::string>(table.GetFields()[1].value), "alice");
    EXPECT_EQ(std::get<std::string>(table.GetFields()[2].value), "alice@example.com");

    // Update
    std::string update_sql = "UPDATE test_users SET balance=200.0 WHERE username='alice'";
    result = m_db->Exec(update_sql);
    EXPECT_TRUE(result.IsOk());

    // Verify update
    TestUsersTable verify_table;
    result = m_db->Exec("SELECT * FROM test_users WHERE username='alice'", verify_table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_DOUBLE_EQ(std::get<double>(verify_table.GetFields()[3].value), 200.0);

    // Delete
    std::string delete_sql = "DELETE FROM test_users WHERE username='alice'";
    result = m_db->Exec(delete_sql);
    EXPECT_TRUE(result.IsOk());

    // Verify deletion
    TestUsersTable check_table;
    result = m_db->Exec("SELECT * FROM test_users WHERE username='alice'", check_table);
    EXPECT_TRUE(result.IsOk());
    // In current implementation, if no rows found, fields remain what they were (monostate/default)
    EXPECT_TRUE(check_table.GetFields()[0].IsNull());
}

// 测试2: 事务处理
TEST_F(DatabaseIntegrationTest, TransactionHandling) {
    // 开始事务
    auto result = m_db->StartTransaction();
    EXPECT_TRUE(result.IsOk());

    // 插入多条记录
    for (int i = 0; i < 5; ++i) {
        std::string sql =
            "INSERT INTO test_users (username, email, balance) VALUES "
            "('user" + std::to_string(i) + "', 'user" + std::to_string(i) + "@test.com', " + std::to_string(i * 10.0) + ")";
        result = m_db->Exec(sql);
        EXPECT_TRUE(result.IsOk());
    }

    // 提交事务
    result = m_db->CommitTransaction();
    EXPECT_TRUE(result.IsOk());

    // 验证所有记录都已插入
    TestUsersTable table;
    result = m_db->Exec("SELECT COUNT(*) as id FROM test_users", table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(std::get<int>(table.GetFields()[0].value), 5);
}

// 测试3: 事务回滚
TEST_F(DatabaseIntegrationTest, TransactionRollback) {
    // 先插入一条记录作为基准
    m_db->Exec("INSERT INTO test_users (username, email) VALUES ('baseline', 'base@test.com')");

    // 开始事务
    auto result = m_db->StartTransaction();
    EXPECT_TRUE(result.IsOk());

    // 插入记录
    m_db->Exec("INSERT INTO test_users (username, email) VALUES ('temp1', 't1@test.com')");
    m_db->Exec("INSERT INTO test_users (username, email) VALUES ('temp2', 't2@test.com')");

    // 回滚事务
    result = m_db->RollbackTransaction();
    EXPECT_TRUE(result.IsOk());

    // 验证只有baseline记录存在
    TestUsersTable table;
    result = m_db->Exec("SELECT COUNT(*) as id FROM test_users", table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(std::get<int>(table.GetFields()[0].value), 1);
}

// 测试4: 多线程并发读写
TEST_F(DatabaseIntegrationTest, ConcurrentReadWrite) {
    const int num_threads = 10;
    const int operations_per_thread = 50;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    // 创建多个线程执行插入操作
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, &success_count, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string username = "user_" + std::to_string(t) + "_" + std::to_string(i);
                std::string email = username + "@test.com";

                std::string sql =
                    "INSERT INTO test_users (username, email, balance) VALUES "
                    "('" + username + "', '" + email + "', " + std::to_string(i * 1.0) + ")";

                auto result = m_db->Exec(sql);
                if (result.IsOk()) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证所有操作成功
    EXPECT_EQ(success_count, num_threads * operations_per_thread);

    // 验证数据库中的记录数
    TestUsersTable table;
    auto result = m_db->Exec("SELECT COUNT(*) as id FROM test_users", table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(std::get<int>(table.GetFields()[0].value), num_threads * operations_per_thread);
}

// 测试5: 使用线程池执行数据库查询
TEST_F(DatabaseIntegrationTest, ThreadPoolDatabaseQueries) {
    // 先插入测试数据
    for (int i = 0; i < 100; ++i) {
        std::string sql =
            "INSERT INTO test_users (username, email, balance) VALUES "
            "('pooluser" + std::to_string(i) + "', 'pool" + std::to_string(i) + "@test.com', " + std::to_string(i * 5.0) + ")";
        m_db->Exec(sql);
    }

    CThreadPool pool;
    pool.Start(4);

    std::atomic<int> query_success{0};
    const int num_queries = 50;

    for (int i = 0; i < num_queries; ++i) {
        pool.AddTask([this, i, &query_success]() {
            std::string sql = "SELECT * FROM test_users WHERE username='pooluser" + std::to_string(i) + "'";
            TestUsersTable table;

            auto result = m_db->Exec(sql, table);
            if (result.IsOk() && !table.GetFields()[1].IsNull()) {
                query_success++;
            }
        });
    }

    pool.Stop();

    EXPECT_EQ(query_success, num_queries);
}

// 测试6: 错误处理 - SQL语法错误
TEST_F(DatabaseIntegrationTest, SQLSyntaxError) {
    std::string invalid_sql = "SELCT * FORM test_users";  // 故意写错
    TestUsersTable table;

    auto result = m_db->Exec(invalid_sql, table);
    EXPECT_FALSE(result.IsOk());
    EXPECT_EQ(result.Error().code, ErrorCode::DatabaseQueryFailed);
}

// 测试7: 大批量数据插入与查询
TEST_F(DatabaseIntegrationTest, BulkInsertAndQuery) {
    const int bulk_size = 1000;

    // 使用事务提高批量插入性能
    auto result = m_db->StartTransaction();
    EXPECT_TRUE(result.IsOk());

    for (int i = 0; i < bulk_size; ++i) {
        std::string sql =
            "INSERT INTO test_users (username, email, balance, created_at) VALUES "
            "('bulk" + std::to_string(i) + "', 'bulk" + std::to_string(i) + "@test.com', " +
            std::to_string(i * 0.01) + ", " + std::to_string(1000000 + i) + ")";

        result = m_db->Exec(sql);
        ASSERT_TRUE(result.IsOk());
    }

    result = m_db->CommitTransaction();
    EXPECT_TRUE(result.IsOk());

    // 验证总数
    TestUsersTable count_table;
    result = m_db->Exec("SELECT COUNT(*) as id FROM test_users", count_table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(std::get<int>(count_table.GetFields()[0].value), bulk_size);

    // 测试范围查询
    TestUsersTable range_table;
    result = m_db->Exec("SELECT COUNT(*) as id FROM test_users WHERE balance > 5.0", range_table);
    EXPECT_TRUE(result.IsOk());
    EXPECT_GT(std::get<int>(range_table.GetFields()[0].value), 0);
}
