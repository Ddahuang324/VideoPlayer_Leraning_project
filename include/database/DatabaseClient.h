#pragma once

#include "Table.h"
#include "common/Result.h"
#include "common/Error.h"
#include "common/Public.h"
#include <map>
#include <string>

namespace yibo {

// 连接参数类型
using KeyValue = std::map<std::string, std::string>;

class CDatabaseClient {
public:
    virtual ~CDatabaseClient() = default;

    // 连接数据库
    virtual Result<void, Error> Connect(const KeyValue& args) = 0;

    // 执行SQL（无返回结果）
    virtual Result<void, Error> Exec(const std::string& sql) = 0;

    // 执行SQL（有返回结果）
    virtual Result<void, Error> Exec(const std::string& sql, Table& table) = 0;

    // 事务控制
    virtual Result<void, Error> StartTransaction() = 0;
    virtual Result<void, Error> CommitTransaction() = 0;
    virtual Result<void, Error> RollbackTransaction() = 0;

    // 关闭连接
    virtual Result<void, Error> Close() = 0;

    // 检查连接状态
    virtual bool IsConnected() const = 0;
};

} // namespace yibo
