#include <gtest/gtest.h>
#include "common/ErrorCode.h"
#include "common/Error.h"

using namespace yibo;

TEST(ErrorCodeTest, ErrorCodeToString) {
    EXPECT_EQ(ErrorCodeToString(ErrorCode::Success), "Success");
    EXPECT_EQ(ErrorCodeToString(ErrorCode::InvalidArgument), "Invalid argument");
    EXPECT_EQ(ErrorCodeToString(ErrorCode::NetworkError), "Network error");
    EXPECT_EQ(ErrorCodeToString(ErrorCode::HttpNotFound), "Not found");
    EXPECT_EQ(ErrorCodeToString(ErrorCode::DatabaseError), "Database error");
}

TEST(ErrorTest, BasicConstruction) {
    Error err(ErrorCode::InvalidArgument, "Test message");
    EXPECT_EQ(err.code, ErrorCode::InvalidArgument);
    EXPECT_EQ(err.message, "Test message");
    EXPECT_TRUE(err.details.empty());
}

TEST(ErrorTest, ConstructionWithDetails) {
    Error err(ErrorCode::NetworkError, "Connection failed", "Timeout after 30s");
    EXPECT_EQ(err.code, ErrorCode::NetworkError);
    EXPECT_EQ(err.message, "Connection failed");
    EXPECT_EQ(err.details, "Timeout after 30s");
}

TEST(ErrorTest, DefaultMessage) {
    Error err(ErrorCode::FileNotFound);
    EXPECT_EQ(err.code, ErrorCode::FileNotFound);
    EXPECT_EQ(err.message, "File not found");
}

TEST(ErrorTest, ToString) {
    Error err1(ErrorCode::InvalidArgument, "Test error");
    EXPECT_EQ(err1.ToString(), "Test error");

    Error err2(ErrorCode::NetworkError, "Connection failed", "Timeout");
    EXPECT_EQ(err2.ToString(), "Connection failed (Details: Timeout)");
}
