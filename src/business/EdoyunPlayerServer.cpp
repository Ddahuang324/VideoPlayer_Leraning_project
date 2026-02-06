#include "business/EdoyunPlayerServer.h"
#include "business/UserTable.h"
#include "http/CHttpParser.h"
#include "http/UrlParser.h"
#include "http/HttpResponse.h"
#include "database/MysqlClient.h"
#include "utils/Crypto.h"
#include "logger/Logger.h"
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
    // 数据库已在 main.cpp 中初始化并连接，此处无需重复连接
    // 如果需要执行某些后台业务逻辑，可以在此处启动
    TRACEI("Business process loop started");
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

    // \u89e3\u6790URL\uff0c\u63d0\u53d6\u8def\u5f84
    UrlParser url_parser(parser.Url());
    BufferView path = url_parser.Path();
    
    // \u8def\u7531\u5206\u53d1
    if (path == "/login" || path == "/api/login") {
        return HandleLogin(pClient, url_parser);
    }
    else if (path == "/register" || path == "/api/register") {
        return HandleRegister(pClient, url_parser);
    }
    else if (path == "/health" || path == "/api/health") {
        return HandleHealth(pClient);
    }
    else {
        return HandleNotFound(pClient, path);
    }
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

Buffer CEdoyunPlayerServer::MakeResponse(int status, const std::string& message, const nlohmann::json& data) {
    nlohmann::json json_body;
    json_body["status"] = status;
    json_body["message"] = message;
    json_body["data"] = data;

    std::string body = json_body.dump();

    std::ostringstream response;
    response << "HTTP/1.1 " << status << " OK\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

// \u767b\u5f55\u5904\u7406\u5668
Result<void, Error> CEdoyunPlayerServer::HandleLogin(CSocketBase* pClient, UrlParser& url_parser) {
    // \u63d0\u53d6\u53c2\u6570
    BufferView user = url_parser["user"];
    BufferView time = url_parser["time"];
    BufferView salt = url_parser["salt"];
    BufferView sign = url_parser["sign"];

    if (user.empty() || time.empty() || salt.empty() || sign.empty()) {
        Buffer response = MakeResponse(400, "Missing parameters");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Missing parameters"));
    }

    // \u67e5\u8be2\u6570\u636e\u5e93
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

    // \u9a8c\u8bc1\u7b7e\u540d
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

    // 
    nlohmann::json user_data;
    user_data["username"] = std::string(user);
    user_data["login_time"] = std::string(time);
    
    Buffer response = MakeResponse(200, "Login successful", user_data);
    (void)pClient->Send(response);

    return Result<void, Error>::Ok();
}

// \u6ce8\u518c\u5904\u7406\u5668
Result<void, Error> CEdoyunPlayerServer::HandleRegister(CSocketBase* pClient, UrlParser& url_parser) {
    BufferView user = url_parser["user"];
    BufferView password = url_parser["password"];

    if (user.empty() || password.empty()) {
        Buffer response = MakeResponse(400, "Missing parameters (user, password required)");
        (void)pClient->Send(response);
        return Result<void, Error>::Err(Error(ErrorCode::InvalidArgument, "Missing parameters"));
    }

    // TODO: \u5b9e\u73b0\u6ce8\u518c\u903b\u8f91\uff08\u63d2\u5165\u6570\u636e\u5e93\uff09
    // \u76ee\u524d\u8fd4\u56de\u672a\u5b9e\u73b0
    Buffer response = MakeResponse(501, "Register feature not implemented yet");
    (void)pClient->Send(response);
    
    return Result<void, Error>::Ok();
}

// \u5065\u5eb7\u68c0\u67e5\u5904\u7406\u5668
Result<void, Error> CEdoyunPlayerServer::HandleHealth(CSocketBase* pClient) {
    nlohmann::json health_data;
    health_data["status"] = "healthy";
    health_data["service"] = "YiboServer";
    health_data["version"] = "1.0.0";
    
    Buffer response = MakeResponse(200, "OK", health_data);
    (void)pClient->Send(response);
    
    return Result<void, Error>::Ok();
}

// 404 \u5904\u7406\u5668
Result<void, Error> CEdoyunPlayerServer::HandleNotFound(CSocketBase* pClient, BufferView path) {
    nlohmann::json error_data;
    error_data["path"] = std::string(path);
    error_data["available_endpoints"] = nlohmann::json::array({"/login", "/register", "/health"});
    
    Buffer response = MakeResponse(404, "Endpoint not found", error_data);
    (void)pClient->Send(response);
    
    return Result<void, Error>::Err(Error(ErrorCode::HttpNotFound, "Endpoint not found"));
}

} // namespace yibo
