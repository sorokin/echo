#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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

struct http_parsing_error : std::runtime_error
{
    explicit http_parsing_error(const std::string& message)
        : runtime_error(message.c_str())
    {}

    explicit http_parsing_error(const char* message)
        : runtime_error(message)
    {}
};

bool http_is_whitespace(char c);
bool http_is_digit(char c);

http_request_method parse_request_method(sub_string& text);
http_version parse_version(sub_string& text);
http_request_line parse_request_line(sub_string& text);
unsigned parse_status_code(sub_string& text);
http_status_line parse_status_line(sub_string& text);
sub_string parse_token(sub_string& text);
void skip_leading_whitespace(sub_string& text);
void parse_field_content(sub_string& text, std::string& target);
void parse_headers(sub_string& text, std::map<std::string, std::vector<std::string> >& headers);
http_request parse_request(sub_string& text);
http_response parse_response(sub_string& text);

#endif // HTTP_PARSER_H
