#include <gtest/gtest.h>
#include "common/Public.h"

using namespace yibo;

TEST(BufferTest, BasicOperations) {
    Buffer buf = "hello";
    EXPECT_EQ(buf.size(), 5);
    EXPECT_EQ(buf, "hello");

    buf += " world";
    EXPECT_EQ(buf, "hello world");
}

TEST(BufferTest, CStyleConversion) {
    Buffer buf = "test";
    const char* ptr = buf.c_str();
    EXPECT_STREQ(ptr, "test");
}

TEST(BufferTest, MoveSemantics) {
    Buffer buf1 = "hello";
    Buffer buf2 = std::move(buf1);
    EXPECT_EQ(buf2, "hello");
    EXPECT_TRUE(buf1.empty());
}

TEST(BufferTest, EmptyBuffer) {
    Buffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0);
}
