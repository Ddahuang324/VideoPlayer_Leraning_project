#include <gtest/gtest.h>
#include "common/Optional.h"

using namespace yibo;

TEST(OptionalTest, DefaultConstructor) {
    Optional<int> opt;
    EXPECT_FALSE(opt.HasValue());
}

TEST(OptionalTest, ValueConstructor) {
    Optional<int> opt(42);
    EXPECT_TRUE(opt.HasValue());
    EXPECT_EQ(opt.Value(), 42);
}

TEST(OptionalTest, NulloptConstructor) {
    Optional<int> opt(std::nullopt);
    EXPECT_FALSE(opt.HasValue());
}

TEST(OptionalTest, ValueOr) {
    Optional<int> opt1(42);
    EXPECT_EQ(opt1.ValueOr(0), 42);

    Optional<int> opt2;
    EXPECT_EQ(opt2.ValueOr(99), 99);
}

TEST(OptionalTest, Map) {
    Optional<int> opt(10);
    auto result = opt.Map([](int x) { return x * 2; });

    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 20);
}

TEST(OptionalTest, MapOnEmpty) {
    Optional<int> opt;
    auto result = opt.Map([](int x) { return x * 2; });

    EXPECT_FALSE(result.HasValue());
}

TEST(OptionalTest, Filter) {
    Optional<int> opt(10);
    auto result = opt.Filter([](int x) { return x > 5; });

    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 10);
}

TEST(OptionalTest, FilterFails) {
    Optional<int> opt(10);
    auto result = opt.Filter([](int x) { return x > 20; });

    EXPECT_FALSE(result.HasValue());
}

TEST(OptionalTest, ChainedOperations) {
    Optional<int> opt(10);
    auto result = opt
        .Map([](int x) { return x * 2; })
        .Filter([](int x) { return x > 15; })
        .Map([](int x) { return x + 5; });

    EXPECT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 25);
}
