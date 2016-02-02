#include "http_common.h"

char const* status_code_as_string(http_status_code status_code)
{
    switch (status_code)
    {
    case http_status_code::ok:
        return "OK";
    case http_status_code::not_modified:
        return "Not Modified";
    case http_status_code::bad_request:
        return "Bad Request";
    case http_status_code::internal_server_error:
        return "Internal Server Error";
    case http_status_code::not_implemented:
        return "Not Implemented";
    default:
        return "Unknown Status Code";
    }
}
