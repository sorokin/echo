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

enum class http_status_code : unsigned
{
    ok                      = 200,

    not_modified            = 304,

    bad_request             = 400,

    internal_server_error   = 500,
    not_implemented         = 501,
};

struct http_status_line
{
    http_version version;
    http_status_code status_code;
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

char const* status_code_as_string(http_status_code);

#endif
