#include "throw_error.h"

#include "errno.h"
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace
{
    char const* error_enum_name(int err)
    {
        switch (err)
        {
        case EBADF:
            return "EBADF";
        case EAGAIN:
            return "EAGAIN";
        case EACCES:
            return "EACCES";
        case EINVAL:
            return "EINVAL";
        case EMFILE:
            return "EMFILE";
        case EPIPE:
            return "EPIPE";
        case EADDRINUSE:
            return "EADDRINUSE";
        case EADDRNOTAVAIL:
            return "EADDRNOTAVAIL";
        case ECONNRESET:
            return "ECONNRESET";
        case ECONNREFUSED:
            return "ECONNREFUSED";
        }

        return "<unknown error>";
    }
}

void throw_error [[noreturn]] (int err, char const* action)
{
    std::stringstream ss;
    ss << action << " failed, error: " << error_enum_name(err);
    char const* err_msg = strerror(err);
    ss << " (" << err << ", " << err_msg << ")";
    throw std::runtime_error(ss.str());
}
