#include "http_printer.h"
#include <cassert>

namespace
{
    char const* request_method_string(http_request_method method)
    {
        switch (method)
        {
        case http_request_method::OPTIONS:
            return "OPTIONS";
        case http_request_method::GET:
            return "GET";
        case http_request_method::HEAD:
            return "HEAD";
        case http_request_method::POST:
            return "POST";
        case http_request_method::PUT:
            return "PUT";
        case http_request_method::DELETE:
            return "DELETE";
        case http_request_method::TRACE:
            return "TRACE";
        case http_request_method::CONNECT:
            return "CONNECT";
        default:
            assert(false);
            return "GET";
        }
    }

    const char* http_version_string(http_version version)
    {
        switch (version)
        {
        case http_version::HTTP_10:
            return "HTTP/1.0";
        case http_version::HTTP_11:
            return "HTTP/1.1";
        default:
            assert(false);
            return "HTTP/1.0";
        }
    }

    void dump_headers(std::ostream& os, std::map<std::string, std::vector<std::string> > const& headers)
    {
        for (auto const& p : headers)
        {
            for (auto const& v : p.second)
            {
                os << p.first << ": " << v << '\n';
            }
        }
    }
}

std::ostream& operator<<(std::ostream& os, sub_string str)
{
    os.write(str.data(), str.size());
    return os;
}

std::ostream& operator<<(std::ostream& os, http_request_method method)
{
    os << request_method_string(method);
    return os;
}


std::ostream& operator<<(std::ostream& os, http_version version)
{
    os << http_version_string(version);
    return os;
}


std::ostream& operator<<(std::ostream& os, http_status_line const& status_line)
{
    os << status_line.version << ' '
       << static_cast<unsigned>(status_line.status_code) << ' '
       << status_line.reason_phrase;
    return os;
}


std::ostream& operator<<(std::ostream& os, http_response const& response)
{
    os << response.status_line << '\n';
    dump_headers(os, response.headers);
    os << '\n';

    return os;
}

std::ostream& operator<<(std::ostream& os, http_request_line const& request_line)
{
    os << request_line.method << ' '
       << request_line.uri << ' '
       << request_line.version;
    return os;
}

std::ostream& operator<<(std::ostream& os, http_request const& request)
{
    os << request.request_line;
    dump_headers(os, request.headers);
    os << '\n';

    return os;
}
