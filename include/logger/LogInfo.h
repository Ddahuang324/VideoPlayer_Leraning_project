#pragma once

#include "common/Result.h"
#include "common/Error.h"
#include "common/Public.h"
#include "common/Optional.h"
#include <chrono>
#include <string>
#include <cstdint>

// 取消系统宏定义以避免冲突
#ifdef DEBUG
#undef DEBUG
#endif
#ifdef ERROR
#undef ERROR
#endif
#ifdef WARNING
#undef WARNING
#endif

namespace yibo {

enum class LogLevel : uint8_t {
    INFO = 0,
    DEBUG = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

struct LogInfo {
    LogLevel level;
    std::chrono::system_clock::time_point timestamp;
    std::string file;
    int line;
    std::string function;
    Buffer content;
    Optional<Buffer> dump_data;

    LogInfo(LogLevel lv, const char* f, int l, const char* func, const char* fmt, ...);

    Result<Buffer, Error> Serialize() const;
    static Result<std::pair<LogInfo, size_t>, Error> Deserialize(BufferView data);

private:
    static std::chrono::system_clock::time_point GetCachedTimestamp();
};

const char* LevelToString(LogLevel level);

} // namespace yibo
