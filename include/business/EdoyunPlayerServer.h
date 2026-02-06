#pragma once


#include "Business.h"
#include "database/DatabaseClient.h"
#include <memory>
#include "nlohmann/json.hpp"

namespace yibo {

class UrlParser;

class CEdoyunPlayerServer : public CBusiness {
public:
    explicit CEdoyunPlayerServer(size_t thread_count, UniquePtr<CDatabaseClient> db = nullptr);
    ~CEdoyunPlayerServer() override = default;

    Result<void, Error> BusinessProcess() override;
    Result<void, Error> Connected(CSocketBase* pClient) override;
    Result<void, Error> Received(CSocketBase* pClient, const Buffer& data) override;
    size_t GetThreadPoolSize() const override { return m_thread_count; }

private:
    Result<void, Error> HttpParser(CSocketBase* pClient, const Buffer& data);
    
    Result<void, Error> HandleLogin(CSocketBase* pClient, UrlParser& url_parser);
    Result<void, Error> HandleRegister(CSocketBase* pClient, UrlParser& url_parser);
    Result<void, Error> HandleHealth(CSocketBase* pClient);
    Result<void, Error> HandleNotFound(CSocketBase* pClient, BufferView path);

    Buffer MakeResponse(int status, const std::string& message);
    Buffer MakeResponse(int status, const std::string& message, const nlohmann::json& data);

private:
    size_t m_thread_count;
    UniquePtr<CDatabaseClient> m_db;
};

} // namespace yibo
