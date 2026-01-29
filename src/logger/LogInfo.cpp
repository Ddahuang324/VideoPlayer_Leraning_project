#include "logger/LogInfo.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace yibo {

namespace {
    // Helper to write values in Big-Endian
    template<typename T>
    void WriteVal(Buffer& buf, T val) {
        for (int i = (int)sizeof(T) - 1; i >= 0; --i) {
            buf.push_back(static_cast<char>((static_cast<uint64_t>(val) >> (i * 8)) & 0xFF));//0xFF只保留最低位，清楚其他位
        }
    }

    // Helper to write string with 4-byte length prefix
    void WriteStr(Buffer& buf, const std::string& str) {
        WriteVal<uint32_t>(buf, static_cast<uint32_t>(str.size()));
        buf.append(str);
    }

    // Helper to read values in Big-Endian
    template<typename T>
    bool ReadVal(BufferView data, size_t& offset, T& val) {
        if (offset + sizeof(T) > data.size()) return false;
        uint64_t uval = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            uval = (uval << 8) | static_cast<uint8_t>(data[offset++]);
        }
        val = static_cast<T>(uval);
        return true;
    }

    // Helper to read string with 4-byte length prefix
    bool ReadStr(BufferView data, size_t& offset, std::string& str) {
        uint32_t len;
        if (!ReadVal(data, offset, len)) return false;
        if (offset + len > data.size()) return false;
        str = std::string(data.substr(offset, len));
        offset += len;
        return true;
    }
}


std::chrono::system_clock::time_point LogInfo::GetCachedTimestamp() {
    static thread_local struct {
        std::chrono::system_clock::time_point cached_time;
        time_t cached_second{0};
    } cache;

    time_t now_sec = time(nullptr);
    if (now_sec != cache.cached_second) {
        cache.cached_time = std::chrono::system_clock::now();
        cache.cached_second = now_sec;
    }

    return cache.cached_time;
}

LogInfo::LogInfo(LogLevel lv, const char* f, int l, const char* func, const char* fmt, ...)
    : level(lv), file(f), line(l), function(func) {

    timestamp = GetCachedTimestamp();

    va_list args;
    va_start(args, fmt);
    char* buf = nullptr;
    int ret = vasprintf(&buf, fmt, args);
    va_end(args);

    if (ret >= 0 && buf) {
        content = Buffer(buf);
        free(buf);
    }
}

Result<Buffer, Error> LogInfo::Serialize() const {
    Buffer result;
    // 预留空间减少重新分配
    result.reserve(256 + content.size() + (dump_data.HasValue() ? dump_data.Value().size() : 0));

    WriteVal(result, static_cast<uint8_t>(level));
    WriteVal(result, static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count()));
    
    WriteStr(result, file);
    WriteVal(result, static_cast<uint32_t>(line));
    WriteStr(result, function);
    WriteStr(result, content);

    if (dump_data.HasValue()) {
        WriteVal(result, uint8_t(1));
        WriteStr(result, dump_data.Value());
    } else {
        WriteVal(result, uint8_t(0));
    }

    return Result<Buffer, Error>::Ok(std::move(result));
}

Result<std::pair<LogInfo, size_t>, Error> LogInfo::Deserialize(BufferView data) {
    size_t offset = 0;
    LogInfo info(LogLevel::INFO, "", 0, "", "");

    auto make_err = [](const char* msg) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, msg));
    };

    // 反序列化各个字段
    uint8_t lv;
    if (!ReadVal(data, offset, lv)) return make_err("Data too short");
    info.level = static_cast<LogLevel>(lv);

    int64_t ts;
    if (!ReadVal(data, offset, ts)) return make_err("Invalid timestamp");
    info.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ts));

    if (!ReadStr(data, offset, info.file)) return make_err("Invalid file path");

    uint32_t line_val;
    if (!ReadVal(data, offset, line_val)) return make_err("Invalid line number");
    info.line = static_cast<int>(line_val);

    if (!ReadStr(data, offset, info.function)) return make_err("Invalid function name");
    if (!ReadStr(data, offset, info.content)) return make_err("Invalid content");

    uint8_t has_dump;
    if (!ReadVal(data, offset, has_dump)) return make_err("Missing dump flag");
    if (has_dump) {
        std::string dump;
        if (!ReadStr(data, offset, dump)) return make_err("Invalid dump data");
        info.dump_data = std::move(dump);
    }

    return Result<std::pair<LogInfo, size_t>, Error>::Ok({std::move(info), offset});
}

const char* LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO ";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKN ";
    }
}

} // namespace yibo
