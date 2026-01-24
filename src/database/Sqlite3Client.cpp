#include "database/Sqlite3Client.h"
#include "common/ErrorCode.h"

namespace yibo {

CSqlite3Client::CSqlite3Client() : m_db(nullptr) {}

CSqlite3Client::~CSqlite3Client() {
    (void)Close();
}

Result<void, Error> CSqlite3Client::Connect(const KeyValue& args) {
    LockGuard<Mutex> lock(m_mutex);

    auto path_it = args.find("path");
    if (path_it == args.end()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Missing 'path' parameter")
        );
    }

    int rc = sqlite3_open(path_it->second.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::string error_msg = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseConnectionFailed, error_msg)
        );
    }

    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
    return Result<void, Error>::Ok();
}

Result<void, Error> CSqlite3Client::Exec(const std::string& sql) {
    LockGuard<Mutex> lock(m_mutex);

    if (!IsConnected()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseError, "Not connected")
        );
    }

    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::string err(error_msg ? error_msg : "Unknown error");
        sqlite3_free(error_msg);
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, err)
        );
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CSqlite3Client::Exec(const std::string& sql, Table& table) {
    LockGuard<Mutex> lock(m_mutex);

    if (!IsConnected()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseError, "Not connected")
        );
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, sqlite3_errmsg(m_db))
        );
    }

    auto result = ParseResultSet(stmt, table);
    sqlite3_finalize(stmt);
    return result;
}

Result<void, Error> CSqlite3Client::ParseResultSet(sqlite3_stmt* stmt, Table& table) {
    auto& fields = table.GetFields();
    int num_cols = sqlite3_column_count(stmt);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        for (int i = 0; i < num_cols && i < static_cast<int>(fields.size()); ++i) {
            int col_type = sqlite3_column_type(stmt, i);

            if (col_type == SQLITE_NULL) {
                fields[i].SetValue(std::monostate{});
            } else {
                const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                if (text) {
                    auto load_result = fields[i].LoadFromStr(text);
                    if (load_result.IsErr()) {
                        return load_result;
                    }
                } else {
                    fields[i].SetValue(std::monostate{});
                }
            }
        }
    } else if (rc != SQLITE_DONE) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, sqlite3_errmsg(m_db))
        );
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CSqlite3Client::StartTransaction() {
    return Exec("BEGIN TRANSACTION");
}

Result<void, Error> CSqlite3Client::CommitTransaction() {
    return Exec("COMMIT");
}

Result<void, Error> CSqlite3Client::RollbackTransaction() {
    return Exec("ROLLBACK");
}

Result<void, Error> CSqlite3Client::Close() {
    LockGuard<Mutex> lock(m_mutex);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    return Result<void, Error>::Ok();
}

bool CSqlite3Client::IsConnected() const {
    return m_db != nullptr;
}

} // namespace yibo
