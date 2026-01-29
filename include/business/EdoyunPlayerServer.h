#pragma once

#include "Business.h"
#include "database/DatabaseClient.h"
#include <memory>

namespace yibo {

class CEdoyunPlayerServer : public CBusiness {
public:
    explicit CEdoyunPlayerServer(size_t thread_count, UniquePtr<CDatabaseClient> db = nullptr);
    ~CEdoyunPlayerServer() override = default;

    // 实现CBusiness接口
    Result<void, Error> BusinessProcess() override;
    Result<void, Error> Connected(CSocketBase* pClient) override;
    Result<void, Error> Received(CSocketBase* pClient, const Buffer& data) override;
    size_t GetThreadPoolSize() const override { return m_thread_count; }

private:
    // HTTP请求解析
    Result<void, Error> HttpParser(CSocketBase* pClient, const Buffer& data);

    // 构造JSON响应
    Buffer MakeResponse(int status, const std::string& message);

private:
    size_t m_thread_count;
    UniquePtr<CDatabaseClient> m_db;
};

} // namespace yibo
