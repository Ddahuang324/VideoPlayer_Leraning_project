#pragma once

#include "DatabaseClient.h"
#include "common/Public.h"
#include <sqlite3.h>

namespace yibo {

class CSqlite3Client : public CDatabaseClient {
private:
    sqlite3* m_db;
    Mutex m_mutex;

public:
    CSqlite3Client();
    ~CSqlite3Client() override;

    Result<void, Error> Connect(const KeyValue& args) override;
    Result<void, Error> Exec(const std::string& sql) override;
    Result<void, Error> Exec(const std::string& sql, Table& table) override;
    Result<void, Error> StartTransaction() override;
    Result<void, Error> CommitTransaction() override;
    Result<void, Error> RollbackTransaction() override;
    Result<void, Error> Close() override;
    bool IsConnected() const override;

private:
    Result<void, Error> ParseResultSet(sqlite3_stmt* stmt, Table& table);
};

} // namespace yibo
