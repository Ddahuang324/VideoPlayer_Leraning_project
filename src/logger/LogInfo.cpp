#include "logger/LogInfo.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace yibo {

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

    // Reserve approximate size
    result.reserve(256 + content.size() + (dump_data.HasValue() ? dump_data.Value().size() : 0));

    // Serialize level (1 byte)
    result.push_back(static_cast<char>(level));

    // Serialize timestamp (8 bytes)
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    for (int i = 7; i >= 0; --i) {
        result.push_back(static_cast<char>((ts >> (i * 8)) & 0xFF));
    }

    // Serialize file path (4 bytes length + data)
    uint32_t file_len = file.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((file_len >> (i * 8)) & 0xFF));
    }
    result.append(file);

    // Serialize line number (4 bytes)
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((line >> (i * 8)) & 0xFF));
    }

    // Serialize function name (4 bytes length + data)
    uint32_t func_len = function.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((func_len >> (i * 8)) & 0xFF));
    }
    result.append(function);

    // Serialize content (4 bytes length + data)
    uint32_t content_len = content.size();
    for (int i = 3; i >= 0; --i) {
        result.push_back(static_cast<char>((content_len >> (i * 8)) & 0xFF));
    }
    result.append(content);

    // Serialize dump data (1 byte has_dump + optional 4 bytes length + data)
    if (dump_data.HasValue()) {
        result.push_back(1);
        uint32_t dump_len = dump_data.Value().size();
        for (int i = 3; i >= 0; --i) {
            result.push_back(static_cast<char>((dump_len >> (i * 8)) & 0xFF));
        }
        result.append(dump_data.Value());
    } else {
        result.push_back(0);
    }

    return Result<Buffer, Error>::Ok(std::move(result));
}

Result<std::pair<LogInfo, size_t>, Error> LogInfo::Deserialize(BufferView data) {
    if (data.size() < 9) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Data too short for deserialization"));
    }

    size_t offset = 0;
    LogInfo info(LogLevel::INFO, "", 0, "", "");

    // Deserialize level
    info.level = static_cast<LogLevel>(static_cast<uint8_t>(data[offset++]));

    // Deserialize timestamp
    int64_t ts = 0;
    for (int i = 0; i < 8; ++i) {
        ts = (ts << 8) | static_cast<uint8_t>(data[offset++]);
    }
    info.timestamp = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(ts));

    // Deserialize file path
    if (offset + 4 > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid file length"));
    }
    uint32_t file_len = 0;
    for (int i = 0; i < 4; ++i) {
        file_len = (file_len << 8) | static_cast<uint8_t>(data[offset++]);
    }
    if (offset + file_len > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid file data"));
    }
    info.file = std::string(data.substr(offset, file_len));
    offset += file_len;

    // Deserialize line number
    if (offset + 4 > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid line number"));
    }
    info.line = 0;
    for (int i = 0; i < 4; ++i) {
        info.line = (info.line << 8) | static_cast<uint8_t>(data[offset++]);
    }

    // Deserialize function name
    if (offset + 4 > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid function length"));
    }
    uint32_t func_len = 0;
    for (int i = 0; i < 4; ++i) {
        func_len = (func_len << 8) | static_cast<uint8_t>(data[offset++]);
    }
    if (offset + func_len > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid function data"));
    }
    info.function = std::string(data.substr(offset, func_len));
    offset += func_len;

    // Deserialize content
    if (offset + 4 > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid content length"));
    }
    uint32_t content_len = 0;
    for (int i = 0; i < 4; ++i) {
        content_len = (content_len << 8) | static_cast<uint8_t>(data[offset++]);
    }
    if (offset + content_len > data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Invalid content data"));
    }
    info.content = Buffer(data.substr(offset, content_len));
    offset += content_len;

    // Deserialize dump data
    if (offset >= data.size()) {
        return Result<std::pair<LogInfo, size_t>, Error>::Err(
            Error(ErrorCode::InvalidArgument, "Missing dump flag"));
    }
    bool has_dump = data[offset++] != 0;
    if (has_dump) {
        if (offset + 4 > data.size()) {
            return Result<std::pair<LogInfo, size_t>, Error>::Err(
                Error(ErrorCode::InvalidArgument, "Invalid dump length"));
        }
        uint32_t dump_len = 0;
        for (int i = 0; i < 4; ++i) {
            dump_len = (dump_len << 8) | static_cast<uint8_t>(data[offset++]);
        }
        if (offset + dump_len > data.size()) {
            return Result<std::pair<LogInfo, size_t>, Error>::Err(
                Error(ErrorCode::InvalidArgument, "Invalid dump data"));
        }
        info.dump_data = Optional<Buffer>(Buffer(data.substr(offset, dump_len)));
        offset += dump_len;
    }

    return Result<std::pair<LogInfo, size_t>, Error>::Ok(std::make_pair(std::move(info), offset));
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
