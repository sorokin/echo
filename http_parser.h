#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdexcept>
#include <string>
#include "sub_string.h"
#include "http_common.h"

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
