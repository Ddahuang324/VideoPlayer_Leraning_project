#include <gtest/gtest.h>
#include "common/Result.h"
#include "common/Error.h"

using namespace yibo;

Result<int, Error> Divide(int a, int b) {
    if (b == 0) {
        return Result<int, Error>::Err(Error(ErrorCode::InvalidArgument, "Division by zero"));
    }
    return Result<int, Error>::Ok(a / b);
}

TEST(ResultTest, OkValue) {
    auto result = Result<int, Error>::Ok(42);
    EXPECT_TRUE(result.IsOk());
    EXPECT_FALSE(result.IsErr());
    EXPECT_EQ(result.Value(), 42);
}

TEST(ResultTest, ErrValue) {
    auto result = Result<int, Error>::Err(Error(ErrorCode::InvalidArgument, "Test error"));
    EXPECT_TRUE(result.IsErr());
    EXPECT_FALSE(result.IsOk());
    EXPECT_EQ(result.Error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(result.Error().message, "Test error");
}

TEST(ResultTest, DivideSuccess) {
    auto result = Divide(10, 2);
    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 5);
}

TEST(ResultTest, DivideByZero) {
    auto result = Divide(10, 0);
    EXPECT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::InvalidArgument);
}

TEST(ResultTest, ValueOr) {
    auto ok_result = Result<int, Error>::Ok(42);
    EXPECT_EQ(ok_result.ValueOr(0), 42);

    auto err_result = Result<int, Error>::Err(Error(ErrorCode::Unknown));
    EXPECT_EQ(err_result.ValueOr(99), 99);
}

TEST(ResultTest, Map) {
    auto result = Result<int, Error>::Ok(10)
        .Map([](int x) { return x * 2; });

    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 20);
}

TEST(ResultTest, MapOnError) {
    auto result = Result<int, Error>::Err(Error(ErrorCode::Unknown))
        .Map([](int x) { return x * 2; });

    EXPECT_TRUE(result.IsErr());
}

TEST(ResultTest, AndThen) {
    auto result = Result<int, Error>::Ok(10)
        .AndThen([](int x) { return Divide(x, 2); });

    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 5);
}

TEST(ResultTest, AndThenPropagatesError) {
    auto result = Result<int, Error>::Ok(10)
        .AndThen([](int x) { return Divide(x, 0); });

    EXPECT_TRUE(result.IsErr());
    EXPECT_EQ(result.Error().code, ErrorCode::InvalidArgument);
}

TEST(ResultTest, ChainedOperations) {
    auto result = Result<int, Error>::Ok(100)
        .AndThen([](int x) { return Divide(x, 2); })
        .AndThen([](int x) { return Divide(x, 5); })
        .Map([](int x) { return x * 2; });

    EXPECT_TRUE(result.IsOk());
    EXPECT_EQ(result.Value(), 20);
}
