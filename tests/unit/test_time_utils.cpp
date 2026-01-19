#include <gtest/gtest.h>
#include "utils/TimeUtils.h"
#include <thread>

TEST(TimeUtilsTest, Timestamp) {
    auto ts = TimeUtils::Timestamp();
    EXPECT_GT(ts, 0);
}

TEST(TimeUtilsTest, TimestampMs) {
    auto ts = TimeUtils::TimestampMs();
    EXPECT_GT(ts, 0);
}

TEST(TimeUtilsTest, Format) {
    auto tp = TimeUtils::FromTimestamp(1705651200);
    auto str = TimeUtils::Format(tp, "%Y-%m-%d");
    EXPECT_EQ(str, "2024-01-19");
}

TEST(TimeUtilsTest, Elapsed) {
    auto start = TimeUtils::Now();
    TimeUtils::Sleep(std::chrono::milliseconds(100));
    auto elapsed = TimeUtils::Elapsed<TimeUtils::Milliseconds>(start);
    EXPECT_GE(elapsed, 100);
    EXPECT_LT(elapsed, 200);
}

TEST(TimeUtilsTest, FromTimestamp) {
    int64_t ts = 1705651200;
    auto tp = TimeUtils::FromTimestamp(ts);
    auto ts2 = std::chrono::duration_cast<TimeUtils::Seconds>(
        tp.time_since_epoch()
    ).count();
    EXPECT_EQ(ts, ts2);
}
