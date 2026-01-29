#include <gtest/gtest.h>
#include "common/Exception.h"

using namespace yibo;

TEST(ExceptionTest, NetworkException) {
    try {
        throw NetworkException("Connection failed");
    } catch (const ServerException& e) {
        EXPECT_EQ(e.Code(), ErrorCode::NetworkError);
        EXPECT_STREQ(e.what(), "Connection failed");
    }
}

TEST(ExceptionTest, DatabaseException) {
    try {
        throw DatabaseException("Query failed");
    } catch (const ServerException& e) {
        EXPECT_EQ(e.Code(), ErrorCode::DatabaseError);
        EXPECT_STREQ(e.what(), "Query failed");
    }
}

TEST(ExceptionTest, HttpException) {
    try {
        throw HttpException("Not found", 404);
    } catch (const HttpException& e) {
        EXPECT_EQ(e.Code(), ErrorCode::HttpError);
        EXPECT_EQ(e.StatusCode(), 404);
        EXPECT_STREQ(e.what(), "Not found");
    }
}

TEST(ExceptionTest, LogicException) {
    try {
        throw LogicException("Invalid state");
    } catch (const ServerException& e) {
        EXPECT_EQ(e.Code(), ErrorCode::LogicError);
        EXPECT_STREQ(e.what(), "Invalid state");
    }
}

TEST(ExceptionTest, CatchAsStdException) {
    try {
        throw NetworkException("Test error");
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "Test error");
    }
}
