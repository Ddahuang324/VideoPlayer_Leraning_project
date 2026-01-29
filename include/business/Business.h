#pragma once

#include "common/Public.h"
#include "network/Socket.h"

namespace yibo {

class CBusiness {
public:
    virtual ~CBusiness() = default;

    // 业务进程主循环
    virtual Result<void, Error> BusinessProcess() = 0;

    // 连接建立回调
    virtual Result<void, Error> Connected(CSocketBase* pClient) = 0;

    // 数据接收回调
    virtual Result<void, Error> Received(CSocketBase* pClient, const Buffer& data) = 0;

    // 获取线程池大小
    virtual size_t GetThreadPoolSize() const = 0;
};

} // namespace yibo
