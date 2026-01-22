#include <gtest/gtest.h>
#include "http/CHttpParser.h"
#include "http/UrlParser.h"
#include "http/HttpResponse.h"

using namespace yibo;

TEST(CHttpParserTest, ParseSimpleGetRequest) {
    Buffer request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHttpParser parser;
    auto result = parser.Parser(request);

    ASSERT_TRUE(result.IsOk());
    EXPECT_EQ(parser.Method(), HttpMethod::HTTP_GET);
    EXPECT_EQ(parser.Url(), "/index.html");
    EXPECT_EQ(parser.Version(), "HTTP/1.1");
    EXPECT_EQ(parser.Header("Host"), "localhost");
    EXPECT_TRUE(parser.IsComplete());
}

TEST(CHttpParserTest, ParsePostRequestWithBody) {
    Buffer request = "POST /api/login HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Content-Length: 13\r\n"
                    "\r\n"
                    "user=test&pwd";
    CHttpParser parser;
    auto result = parser.Parser(request);

    ASSERT_TRUE(result.IsOk());
    EXPECT_EQ(parser.Method(), HttpMethod::HTTP_POST);
    EXPECT_EQ(parser.Url(), "/api/login");
    EXPECT_EQ(parser.Body(), "user=test&pwd");
}

TEST(CHttpParserTest, ParseMultipleHeaders) {
    Buffer request = "GET / HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "User-Agent: TestClient\r\n"
                    "Accept: */*\r\n"
                    "\r\n";
    CHttpParser parser;
    auto result = parser.Parser(request);

    ASSERT_TRUE(result.IsOk());
    EXPECT_EQ(parser.Header("Host"), "localhost");
    EXPECT_EQ(parser.Header("User-Agent"), "TestClient");
    EXPECT_EQ(parser.Header("Accept"), "*/*");
}

TEST(CHttpParserTest, InvalidRequestLine) {
    Buffer request = "INVALID\r\n\r\n";
    CHttpParser parser;
    auto result = parser.Parser(request);

    EXPECT_TRUE(result.IsErr());
}

TEST(UrlParserTest, ParsePath) {
    UrlParser parser("/api/users");
    EXPECT_EQ(parser.Path(), "/api/users");
    EXPECT_TRUE(parser.Query().empty());
}

TEST(UrlParserTest, ParseQueryParameters) {
    UrlParser parser("/api/login?user=admin&pass=123");
    EXPECT_EQ(parser.Path(), "/api/login");
    EXPECT_EQ(parser["user"], "admin");
    EXPECT_EQ(parser["pass"], "123");
}

TEST(UrlParserTest, EmptyQueryParameter) {
    UrlParser parser("/api/test");
    EXPECT_TRUE(parser["nonexistent"].empty());
}

TEST(HttpResponseTest, BuildSimpleResponse) {
    HttpResponse response;
    response.SetStatus(200, "OK")
            .AddHeader("Content-Type", "text/plain")
            .SetBody("Hello World");

    Buffer result = response.Build();
    EXPECT_TRUE(result.find("HTTP/1.1 200 OK") != Buffer::npos);
    EXPECT_TRUE(result.find("Content-Type: text/plain") != Buffer::npos);
    EXPECT_TRUE(result.find("Content-Length: 11") != Buffer::npos);
    EXPECT_TRUE(result.find("Hello World") != Buffer::npos);
}

TEST(HttpResponseTest, Build404Response) {
    HttpResponse response;
    response.SetStatus(404, "Not Found")
            .SetBody("Page not found");

    Buffer result = response.Build();
    EXPECT_TRUE(result.find("HTTP/1.1 404 Not Found") != Buffer::npos);
    EXPECT_TRUE(result.find("Page not found") != Buffer::npos);
}
