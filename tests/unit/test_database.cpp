#include <gtest/gtest.h>
#include "database/Field.h"
#include "database/MysqlTable.h"
#include "database/Sqlite3Table.h"
#include "database/Sqlite3Client.h"

using namespace yibo;

// Field类测试
TEST(FieldTest, SetAndGetValue) {
    Field field("test", SqlType::TYPE_INT);
    field.SetValue(42);
    EXPECT_EQ(field.ToSqlString(), "42");
}

TEST(FieldTest, LoadFromString) {
    Field field("test", SqlType::TYPE_INT);
    auto result = field.LoadFromStr("123");
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(field.ToSqlString(), "123");
}

TEST(FieldTest, NullValue) {
    Field field("test", SqlType::TYPE_VARCHAR);
    field.SetValue(std::monostate{});
    EXPECT_TRUE(field.IsNull());
    EXPECT_EQ(field.ToSqlString(), "NULL");
}

TEST(FieldTest, BoolConversion) {
    Field field("test", SqlType::TYPE_BOOL);
    field.SetValue(true);
    EXPECT_EQ(field.ToSqlString(), "1");
    field.SetValue(false);
    EXPECT_EQ(field.ToSqlString(), "0");
}

TEST(FieldTest, StringValue) {
    Field field("test", SqlType::TYPE_VARCHAR);
    field.SetValue(std::string("hello"));
    EXPECT_EQ(field.ToSqlString(), "'hello'");
}

// Table类测试
class TestTable : public Sqlite3Table {
public:
    TestTable() {
        m_fields = {
            Field("id", SqlType::TYPE_INT, PRIMARY_KEY | AUTOINCREMENT),
            Field("name", SqlType::TYPE_VARCHAR, NOT_NULL),
            Field("age", SqlType::TYPE_INT)
        };
    }
    std::string GetTableName() const override { return "test_table"; }
};

TEST(TableTest, CreateSQL) {
    TestTable table;
    std::string sql = table.Create();
    EXPECT_NE(sql.find("CREATE TABLE"), std::string::npos);
    EXPECT_NE(sql.find("test_table"), std::string::npos);
    EXPECT_NE(sql.find("PRIMARY KEY"), std::string::npos);
}

TEST(TableTest, InsertSQL) {
    TestTable table;
    table.GetFields()[1].SetValue(std::string("Alice"));
    table.GetFields()[2].SetValue(25);
    std::string sql = table.Insert();
    EXPECT_NE(sql.find("INSERT INTO"), std::string::npos);
    EXPECT_NE(sql.find("'Alice'"), std::string::npos);
    EXPECT_NE(sql.find("25"), std::string::npos);
}

TEST(TableTest, QuerySQL) {
    TestTable table;
    std::string sql = table.Query("id=1");
    EXPECT_NE(sql.find("SELECT"), std::string::npos);
    EXPECT_NE(sql.find("WHERE id=1"), std::string::npos);
}

TEST(TableTest, DeleteSQL) {
    TestTable table;
    std::string sql = table.Delete("id=1");
    EXPECT_NE(sql.find("DELETE FROM"), std::string::npos);
    EXPECT_NE(sql.find("WHERE id=1"), std::string::npos);
}

// SQLite客户端测试
TEST(Sqlite3ClientTest, ConnectMemoryDB) {
    CSqlite3Client client;
    KeyValue args;
    args["path"] = ":memory:";
    auto result = client.Connect(args);
    EXPECT_TRUE(result.IsOk());
    EXPECT_TRUE(client.IsConnected());
}

TEST(Sqlite3ClientTest, CreateTable) {
    CSqlite3Client client;
    KeyValue args;
    args["path"] = ":memory:";
    client.Connect(args);

    TestTable table;
    auto result = client.Exec(table.Create());
    EXPECT_TRUE(result.IsOk());
}

TEST(Sqlite3ClientTest, InsertAndQuery) {
    CSqlite3Client client;
    KeyValue args;
    args["path"] = ":memory:";
    client.Connect(args);

    TestTable table;
    client.Exec(table.Create());

    table.GetFields()[1].SetValue(std::string("Bob"));
    table.GetFields()[2].SetValue(30);
    auto result = client.Exec(table.Insert());
    EXPECT_TRUE(result.IsOk());

    TestTable queryTable;
    result = client.Exec(queryTable.Query(), queryTable);
    EXPECT_TRUE(result.IsOk());
}

TEST(Sqlite3ClientTest, Transaction) {
    CSqlite3Client client;
    KeyValue args;
    args["path"] = ":memory:";
    client.Connect(args);

    auto result = client.StartTransaction();
    EXPECT_TRUE(result.IsOk());

    result = client.CommitTransaction();
    EXPECT_TRUE(result.IsOk());
}

TEST(Sqlite3ClientTest, Rollback) {
    CSqlite3Client client;
    KeyValue args;
    args["path"] = ":memory:";
    client.Connect(args);

    TestTable table;
    client.Exec(table.Create());

    client.StartTransaction();
    table.GetFields()[1].SetValue(std::string("Test"));
    table.GetFields()[2].SetValue(20);
    client.Exec(table.Insert());

    auto result = client.RollbackTransaction();
    EXPECT_TRUE(result.IsOk());
}
