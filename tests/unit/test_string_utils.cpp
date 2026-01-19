#include <gtest/gtest.h>
#include "utils/StringUtils.h"

TEST(StringUtilsTest, Trim) {
    EXPECT_EQ(StringUtils::Trim("  hello  "), "hello");
    EXPECT_EQ(StringUtils::Trim("\t\nhello\r\n"), "hello");
    EXPECT_EQ(StringUtils::Trim(""), "");
}

TEST(StringUtilsTest, Split) {
    auto parts = StringUtils::Split("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(StringUtilsTest, CaseConversion) {
    EXPECT_EQ(StringUtils::ToUpper("hello"), "HELLO");
    EXPECT_EQ(StringUtils::ToLower("WORLD"), "world");
}

TEST(StringUtilsTest, UrlEncode) {
    EXPECT_EQ(StringUtils::UrlEncode("hello world"), "hello%20world");
    EXPECT_EQ(StringUtils::UrlEncode("a+b=c"), "a%2Bb%3Dc");
}

TEST(StringUtilsTest, UrlDecode) {
    EXPECT_EQ(StringUtils::UrlDecode("hello%20world"), "hello world");
    EXPECT_EQ(StringUtils::UrlDecode("a%2Bb%3Dc"), "a+b=c");
}

TEST(StringUtilsTest, StartsWith) {
    EXPECT_TRUE(StringUtils::StartsWith("hello world", "hello"));
    EXPECT_FALSE(StringUtils::StartsWith("hello world", "world"));
}

TEST(StringUtilsTest, EndsWith) {
    EXPECT_TRUE(StringUtils::EndsWith("hello world", "world"));
    EXPECT_FALSE(StringUtils::EndsWith("hello world", "hello"));
}

TEST(StringUtilsTest, Join) {
    std::vector<std::string> items = {"a", "b", "c"};
    EXPECT_EQ(StringUtils::Join(items, ", "), "a, b, c");
}

TEST(StringUtilsTest, Replace) {
    EXPECT_EQ(StringUtils::Replace("hello world", "world", "there"), "hello there");
    EXPECT_EQ(StringUtils::Replace("aaa", "a", "b"), "bbb");
}
