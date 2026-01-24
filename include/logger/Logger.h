#pragma once

#include "logger/LogInfo.h"

namespace yibo {

void Trace(const LogInfo& info);

} // namespace yibo

#define TRACEI(fmt, ...) \
    yibo::Trace(yibo::LogInfo(yibo::LogLevel::INFO, __FILE__, __LINE__, \
                              __FUNCTION__, fmt, ##__VA_ARGS__))

#define TRACED(fmt, ...) \
    yibo::Trace(yibo::LogInfo(yibo::LogLevel::DEBUG, __FILE__, __LINE__, \
                              __FUNCTION__, fmt, ##__VA_ARGS__))

#define TRACEW(fmt, ...) \
    yibo::Trace(yibo::LogInfo(yibo::LogLevel::WARNING, __FILE__, __LINE__, \
                              __FUNCTION__, fmt, ##__VA_ARGS__))

#define TRACEE(fmt, ...) \
    yibo::Trace(yibo::LogInfo(yibo::LogLevel::ERROR, __FILE__, __LINE__, \
                              __FUNCTION__, fmt, ##__VA_ARGS__))

#define TRACEF(fmt, ...) \
    yibo::Trace(yibo::LogInfo(yibo::LogLevel::FATAL, __FILE__, __LINE__, \
                              __FUNCTION__, fmt, ##__VA_ARGS__))

#define DUMPI(data, len, fmt, ...) \
    do { \
        yibo::LogInfo info(yibo::LogLevel::INFO, __FILE__, __LINE__, \
                          __FUNCTION__, fmt, ##__VA_ARGS__); \
        info.dump_data = yibo::Optional<yibo::Buffer>(yibo::Buffer((const char*)(data), (len))); \
        yibo::Trace(info); \
    } while(0)
