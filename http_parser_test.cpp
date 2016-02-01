#include <gtest/gtest.h>
#include "http_parser.h"

TEST(http_request_method_parsing, simple01)
{
    sub_string str = sub_string::literal("GET");
    http_request_method method = parse_request_method(str);
    EXPECT_EQ(method, http_request_method::GET);
    EXPECT_TRUE(str.empty());
}

TEST(http_request_method_parsing, simple02)
{
    sub_string str = sub_string::literal("GET ");
    http_request_method method = parse_request_method(str);
    EXPECT_EQ(method, http_request_method::GET);
    EXPECT_TRUE(!str.empty());
}

TEST(http_request_method_parsing, simple03)
{
    sub_string str = sub_string::literal("POST");
    http_request_method method = parse_request_method(str);
    EXPECT_EQ(method, http_request_method::POST);
    EXPECT_TRUE(str.empty());
}

TEST(http_request_method_parsing, simple04)
{
    sub_string str = sub_string::literal("PUT");
    http_request_method method = parse_request_method(str);
    EXPECT_EQ(method, http_request_method::PUT);
    EXPECT_TRUE(str.empty());
}

TEST(http_request_method_parsing, invalid01)
{
    sub_string str = sub_string::literal("PU");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_request_method_parsing, invalid02)
{
    sub_string str = sub_string::literal("");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_request_method_parsing, invalid03)
{
    sub_string str = sub_string::literal(" ");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_request_method_parsing, invalid04)
{
    sub_string str = sub_string::literal("OPTION");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_version_parsing, simple01)
{
    sub_string str = sub_string::literal("HTTP/1.0");
    http_version version = parse_version(str);
    EXPECT_EQ(version, http_version::HTTP_10);
    EXPECT_TRUE(str.empty());
}

TEST(http_version_parsing, simple02)
{
    sub_string str = sub_string::literal("HTTP/00001.000");
    http_version version = parse_version(str);
    EXPECT_EQ(version, http_version::HTTP_10);
    EXPECT_TRUE(str.empty());
}

TEST(http_version_parsing, simple03)
{
    sub_string str = sub_string::literal("HTTP/1.1");
    http_version version = parse_version(str);
    EXPECT_EQ(version, http_version::HTTP_11);
    EXPECT_TRUE(str.empty());
}

TEST(http_version_parsing, simple04)
{
    sub_string str = sub_string::literal("HTTP/00001.0001");
    http_version version = parse_version(str);
    EXPECT_EQ(version, http_version::HTTP_11);
    EXPECT_TRUE(str.empty());
}

TEST(http_version_parsing, simple05)
{
    sub_string str = sub_string::literal("HTTP/1.0 ");
    http_version version = parse_version(str);
    EXPECT_EQ(version, http_version::HTTP_10);
    EXPECT_TRUE(!str.empty());
}

TEST(http_version_parsing, invalid01)
{
    sub_string str = sub_string::literal("HTTP/");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_version_parsing, invalid02)
{
    sub_string str = sub_string::literal("");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_version_parsing, invalid03)
{
    sub_string str = sub_string::literal(" ");
    EXPECT_THROW(parse_request_method(str), http_parsing_error);
}

TEST(http_request_line_parsing, simple01)
{
    sub_string str = sub_string::literal("OPTIONS * HTTP/1.1\r\n");
    http_request_line request_line = parse_request_line(str);
    EXPECT_EQ(request_line.method, http_request_method::OPTIONS);
    EXPECT_EQ(request_line.version, http_version::HTTP_11);
    EXPECT_TRUE(str.empty());
}

TEST(http_request_parsing, simple01)
{
    sub_string str = sub_string::literal("GET http://ya.ru/ HTTP/1.1\r\nHost: ya.ru\r\n\r\n");
    http_request request = parse_request(str);
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(request.headers["Host"][0], "ya.ru");
}

TEST(http_request_parsing, simple02)
{
    sub_string str = sub_string::literal("GET http://ya.ru/ HTTP/1.1\r\nHost:    ya.ru    \r\n\r\n");
    http_request request = parse_request(str);
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(request.headers["Host"][0], "ya.ru");
}

GTEST_TEST(http_request_parsing, smoke01)
{
    sub_string str = sub_string::literal(
        "GET /Protocols/rfc2616/rfc2616-sec6.html HTTP/1.1" "\r\n"
        "Host: www.w3.org" "\r\n"
        "Connection: keep-alive" "\r\n"
        "Cache-Control: max-age=0" "\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" "\r\n"
        "Upgrade-Insecure-Requests: 1" "\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.106 Safari/537.36" "\r\n"
        "Referer: http://www.w3.org/Protocols/rfc2616/rfc2616.html" "\r\n"
        "Accept-Encoding: gzip, deflate, sdch" "\r\n"
        "Accept-Language: en-US,en;q=0.8,ru;q=0.6" "\r\n"
        "If-None-Match: \"277f-3e3073913b100\"" "\r\n"
        "If-Modified-Since: Wed, 01 Sep 2004 13:24:52 GMT" "\r\n"
        "\r\n");
    http_request request = parse_request(str);
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(request.headers["Host"][0], "www.w3.org");
}

GTEST_TEST(http_request_parsing, smoke02)
{
    sub_string str = sub_string::literal(
        "GET / HTTP/1.1" "\r\n"
        "Host: www.amazon.com" "\r\n"
        "Connection: keep-alive" "\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" "\r\n"
        "Upgrade-Insecure-Requests: 1" "\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.106 Safari/537.36" "\r\n"
        "Accept-Encoding: gzip, deflate, sdch" "\r\n"
        "Accept-Language: en-US,en;q=0.8,ru;q=0.6" "\r\n"
        "Cookie: x-wl-uid=1zRaamWWvwjLwaG9foMMTNlRn5zDbq2Z3n5dqLYljyFV0g6M0AU0f5NXtJJhlL5xhv19oeBFZVyU=; session-\"token=tK09WOPkx9PdUBvaPen7ew0CCDBGvSQ2xwNgELeJQAJMKxClfFAuffra\"+Ysh8nwdiNcs3NFFq59SviRbRMcSQLr2J9XR7wyEtE6in6joFq3AXUUQq0liD7Pflnhw1uERVtJn2hVXtCTgel2h/4F0Wi1XN/CRwKPg/7Q4OU0xoTthQMkkRBTmhxM1jUF6c9/6x2XVX7CXar4Bqp9W5oOHpj5oMH9eTYfcud8uT655xKeDwmLrWw4C9UiS6MpCwtqd; ubid-main=190-5441568-7691748; session-id-time=2082787201l; session-id=178-4697370-5690842" "\r\n"
        "\r\n");
    http_request request = parse_request(str);
    EXPECT_TRUE(str.empty());
    EXPECT_EQ(request.headers["Host"][0], "www.amazon.com");
}

TEST(http_status_code_parsing, simple01)
{
    sub_string str = sub_string::literal("304");
    http_status_code status_code = parse_status_code(str);
    EXPECT_EQ(status_code, http_status_code::not_modified);
    EXPECT_TRUE(str.empty());
}

TEST(http_status_line_parsing, simple01)
{
    sub_string str = sub_string::literal("HTTP/1.1 200 OK\r\n");
    http_status_line status_line = parse_status_line(str);
    EXPECT_EQ(status_line.status_code, http_status_code::ok);
    EXPECT_TRUE(str.empty());
}

TEST(http_response_parsing, smoke01)
{
    sub_string str = sub_string::literal(
        "HTTP/1.1 304 Not Modified" "\r\n"
        "Date: Wed, 23 Dec 2015 13:32:04 GMT" "\r\n"
        "Server: Apache/2" "\r\n"
        "ETag: \"277f-3e3073913b100\"" "\r\n"
        "Expires: Wed, 23 Dec 2015 19:32:04 GMT" "\r\n"
        "Cache-Control: max-age=21600" "\r\n"
        "\r\n");
    http_response response = parse_response(str);
    EXPECT_TRUE(str.empty());
}

TEST(http_response_parsing, smoke02)
{
    sub_string str = sub_string::literal(
        "HTTP/1.1 200 OK" "\r\n"
        "Date: Wed, 23 Dec 2015 13:43:56 GMT" "\r\n"
        "Server: Server" "\r\n"
        "Set-Cookie: skin=noskin; path=/; domain=.amazon.com" "\r\n"
        "pragma: no-cache" "\r\n"
        "x-amz-id-1: 0WDRFNYP6K3XJ8VGHRBQ" "\r\n"
        "p3p: policyref=\"https://www.amazon.com/w3c/p3p.xml\",CP=\"CAO DSP LAW CUR ADM IVAo IVDo CONo OTPo OUR DELi PUBi OTRi BUS PHY ONL UNI PUR FIN COM NAV INT DEM CNT STA HEA PRE LOC GOV OTC \"" "\r\n"
        "cache-control: no-cache" "\r\n"
        "x-frame-options: SAMEORIGIN" "\r\n"
        "expires: -1" "\r\n"
        "Vary: Accept-Encoding,User-Agent,Avail-Dictionary" "\r\n"
        "Content-Encoding: gzip" "\r\n"
        "Content-Type: text/html; charset=UTF-8" "\r\n"
        "Set-cookie: ubid-main=190-5441568-7691748; path=/; domain=.amazon.com; expires=Tue, 01-Jan-2036 08:00:01 GMT" "\r\n"
        "Set-cookie: session-id-time=2082787201l; path=/; domain=.amazon.com; expires=Tue, 01-Jan-2036 08:00:01 GMT" "\r\n"
        "Set-cookie: session-id=178-4697370-5690842; path=/; domain=.amazon.com; expires=Tue, 01-Jan-2036 08:00:01 GMT" "\r\n"
        "Transfer-Encoding: chunked" "\r\n"
        "\r\n");
    http_response response = parse_response(str);
    EXPECT_TRUE(str.empty());
}
