#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#include <map>
#include <string>
#include <vector>
#include "sub_string.h"

enum class http_request_method
{
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    CONNECT,
};

enum class http_version
{
    HTTP_10,
    HTTP_11,
};

struct http_request_line
{
    http_request_method method;
    sub_string uri;
    http_version version;
};

struct http_status_line
{
    http_version version;
    unsigned status_code;
    sub_string reason_phrase;
};

struct http_request
{
    http_request_line request_line;
    std::map<std::string, std::vector<std::string> > headers;
};

struct http_response
{
    http_status_line status_line;
    std::map<std::string, std::vector<std::string> > headers;
};

#endif
