#include "business/EdoyunPlayerServer.h"
#include "business/UserTable.h"
#include "http/CHttpParser.h"
#include "http/UrlParser.h"
#include "http/HttpResponse.h"
#include "database/MysqlClient.h"
#include "utils/Crypto.h"
#include "nlohmann/json.hpp"
#include <sstream>

namespace yibo {

CEdoyunPlayerServer::CEdoyunPlayerServer(size_t thread_count, UniquePtr<CDatabaseClient> db)
    : m_thread_count(thread_count)
    , m_db(std::move(db))
{
    if (!m_db) {
        m_db = MakeUnique<CMysqlClient>();
    }
}

Result<void, Error> CEdoyunPlayerServer::BusinessProcess() {
    // 连接数据库
    KeyValue db_config = {
        {"host", "localhost"},
        {"user", "root"},
        {"password", ""},
        {"db", "edoyun"}
    };

    auto result = m_db->Connect(db_config);
    if (result.IsErr()) {
        return Result<void, Error>::Err(result.Error());
    }

    return Result<void, Error>::Ok();
}

Result<void, Error> CEdoyunPlayerServer::Connected(CSocketBase*) {
    // 连接建立时的处理
    return Result<void, Error>::Ok();
}

Result<void, Error> CEdoyunPlayerServer::Received(CSocketBase* pClient, const Buffer& data) {
    return HttpParser(pClient, data);
}

Result<void, Error> CEdoyunPlayerServer::HttpParser(CSocketBase* pClient, const Buffer& data) {
    CHttpParser parser;
    auto result = parser.Parser(data);
    if (result.IsErr() || !parser.IsComplete()) {
        Buffer response = MakeResponse(400, "Bad Request");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Bad Request"));
    }

    // 解析URL参数
    UrlParser url_parser(parser.Url());
    BufferView user = url_parser["user"];
    BufferView time = url_parser["time"];
    BufferView salt = url_parser["salt"];
    BufferView sign = url_parser["sign"];

    if (user.empty() || time.empty() || salt.empty() || sign.empty()) {
        Buffer response = MakeResponse(400, "Missing parameters");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Missing parameters"));
    }

    // 查询数据库
    UserTable table;
    table.GetFields().push_back(Field("username", SqlType::TYPE_VARCHAR));
    table.GetFields().push_back(Field("password", SqlType::TYPE_VARCHAR));

    std::string escaped_user = m_db->Escape(std::string(user));
    std::ostringstream sql;
    sql << "SELECT username, password FROM users WHERE username='" << escaped_user << "'";

    auto db_result = m_db->Exec(sql.str(), table);
    if (db_result.IsErr() || table.GetFields().empty() || table.GetFields()[0].IsNull()) {
        Buffer response = MakeResponse(404, "User not found");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::HttpNotFound, "User not found"));
    }

    // 验证签名
    auto* password_ptr = std::get_if<std::string>(&table.GetFields()[1].value);
    if (!password_ptr) {
        Buffer response = MakeResponse(500, "Database data error");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::DatabaseError, "Invalid password field"));
    }

    std::string password = *password_ptr;
    std::string expected_sign = Crypto::MD5(std::string(user) + std::string(time) + password + std::string(salt));

    if (expected_sign != sign) {
        Buffer response = MakeResponse(403, "Invalid signature");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::AuthenticationFailed, "Invalid signature"));
    }

    // 构造成功响应
    Buffer response = MakeResponse(200, "Success");
    (void)pClient->Send(response);

    return Result<void, Error>::Ok();
}

Buffer CEdoyunPlayerServer::MakeResponse(int status, const std::string& message) {
    nlohmann::json json_body;
    json_body["status"] = status;
    json_body["message"] = message;

    std::string body = json_body.dump();

    std::ostringstream response;
    response << "HTTP/1.1 " << status << " OK\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

} // namespace yibo
