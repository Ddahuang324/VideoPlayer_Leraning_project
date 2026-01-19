#include "utils/TimeUtils.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace TimeUtils {

std::string Format(const TimePoint& tp, std::string_view fmt) {
    auto time_t_val = Clock::to_time_t(tp);
    std::tm tm_val;
    localtime_r(&time_t_val, &tm_val);

    std::ostringstream oss;
    oss << std::put_time(&tm_val, fmt.data());
    return oss.str();
}

} // namespace TimeUtils
