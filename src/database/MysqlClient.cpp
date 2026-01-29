#include "database/MysqlClient.h"
#include "common/ErrorCode.h"

namespace yibo {

CMysqlClient::CMysqlClient() : m_conn(nullptr) {}

CMysqlClient::~CMysqlClient() {
    (void)Close();
}

Result<void, Error> CMysqlClient::Connect(const KeyValue& args) {
    LockGuard<Mutex> lock(m_mutex);

    m_conn = mysql_init(nullptr);
    if (!m_conn) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseConnectionFailed, "mysql_init failed")
        );
    }

    auto host_it = args.find("host");
    auto user_it = args.find("user");
    auto password_it = args.find("password");
    auto db_it = args.find("db");

    if (host_it == args.end() || user_it == args.end() ||
        password_it == args.end() || db_it == args.end()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Missing connection parameters")
        );
    }

    unsigned int port = 3306;
    auto port_it = args.find("port");
    if (port_it != args.end()) {
        port = std::stoi(port_it->second);
    }

    if (!mysql_real_connect(m_conn, host_it->second.c_str(), user_it->second.c_str(),
                            password_it->second.c_str(), db_it->second.c_str(),
                            port, nullptr, 0)) {
        std::string error_msg = mysql_error(m_conn);
        mysql_close(m_conn);
        m_conn = nullptr;
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseConnectionFailed, error_msg)
        );
    }

    mysql_set_character_set(m_conn, "utf8");
    return Result<void, Error>::Ok();
}

Result<void, Error> CMysqlClient::Exec(const std::string& sql) {
    LockGuard<Mutex> lock(m_mutex);

    if (!IsConnected()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseError, "Not connected")
        );
    }

    if (mysql_query(m_conn, sql.c_str()) != 0) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, mysql_error(m_conn))
        );
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CMysqlClient::Exec(const std::string& sql, Table& table) {
    LockGuard<Mutex> lock(m_mutex);

    if (!IsConnected()) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseError, "Not connected")
        );
    }

    if (mysql_query(m_conn, sql.c_str()) != 0) {
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, mysql_error(m_conn))
        );
    }

    MYSQL_RES* res = mysql_store_result(m_conn);
    if (!res) {
        if (mysql_field_count(m_conn) == 0) {
            return Result<void, Error>::Ok();
        }
        return Result<void, Error>::Err(
            Error(ErrorCode::DatabaseQueryFailed, mysql_error(m_conn))
        );
    }

    auto result = ParseResultSet(res, table);
    mysql_free_result(res);
    return result;
}

Result<void, Error> CMysqlClient::ParseResultSet(MYSQL_RES* res, Table& table) {
    auto& fields = table.GetFields();
    unsigned int num_fields = mysql_num_fields(res);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        for (unsigned int i = 0; i < num_fields && i < fields.size(); ++i) {
            if (row[i]) {
                std::string_view value(row[i], lengths[i]);
                auto load_result = fields[i].LoadFromStr(value);
                if (load_result.IsErr()) {
                    return load_result;
                }
            } else {
                fields[i].SetValue(std::monostate{});
            }
        }
        break;
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CMysqlClient::StartTransaction() {
    return Exec("START TRANSACTION");
}

Result<void, Error> CMysqlClient::CommitTransaction() {
    return Exec("COMMIT");
}

Result<void, Error> CMysqlClient::RollbackTransaction() {
    return Exec("ROLLBACK");
}

Result<void, Error> CMysqlClient::Close() {
    LockGuard<Mutex> lock(m_mutex);
    if (m_conn) {
        mysql_close(m_conn);
        m_conn = nullptr;
    }
    return Result<void, Error>::Ok();
}

bool CMysqlClient::IsConnected() const {
    return m_conn != nullptr;
}

std::string CMysqlClient::Escape(const std::string& str) {
    if (!IsConnected() || str.empty()) {
        return str;
    }

    std::string escaped(str.size() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(m_conn, &escaped[0], str.c_str(), str.size());
    escaped.resize(len);
    return escaped;
}

} // namespace yibo
