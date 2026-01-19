#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace TimeUtils {

// 类型别名
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

// 获取当前时间
inline TimePoint Now() {
    return Clock::now();
}

// 获取时间戳（秒）
inline int64_t Timestamp() {
    return std::chrono::duration_cast<Seconds>(
        Now().time_since_epoch()
    ).count();
}

// 获取时间戳（毫秒）
inline int64_t TimestampMs() {
    return std::chrono::duration_cast<Milliseconds>(
        Now().time_since_epoch()
    ).count();
}

// 格式化时间
std::string Format(const TimePoint& tp, std::string_view fmt = "%Y-%m-%d %H:%M:%S");

// 从时间戳创建TimePoint
inline TimePoint FromTimestamp(int64_t ts) {
    return TimePoint(Seconds(ts));
}

// 时间差计算
template<typename DurationType = Milliseconds>
inline int64_t Elapsed(const TimePoint& start) {
    return std::chrono::duration_cast<DurationType>(
        Now() - start
    ).count();
}

// 睡眠
template<typename DurationType>
inline void Sleep(DurationType duration) {
    std::this_thread::sleep_for(duration);
}

} // namespace TimeUtils
