#pragma once

#include "DatabaseClient.h"
#include "common/Public.h"
#include <mysql/mysql.h>

namespace yibo {

class CMysqlClient : public CDatabaseClient {
private:
    MYSQL* m_conn;
    Mutex m_mutex;

public:
    CMysqlClient();
    ~CMysqlClient() override;

    Result<void, Error> Connect(const KeyValue& args) override;
    Result<void, Error> Exec(const std::string& sql) override;
    Result<void, Error> Exec(const std::string& sql, Table& table) override;
    Result<void, Error> StartTransaction() override;
    Result<void, Error> CommitTransaction() override;
    Result<void, Error> RollbackTransaction() override;
    Result<void, Error> Close() override;
    bool IsConnected() const override;

private:
    Result<void, Error> ParseResultSet(MYSQL_RES* res, Table& table);
};

} // namespace yibo
