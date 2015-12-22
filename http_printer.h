#ifndef HTTP_PRINTER_H
#define HTTP_PRINTER_H

#include <ostream>
#include "http_common.h"

std::ostream& operator<<(std::ostream&, sub_string);
std::ostream& operator<<(std::ostream&, http_request_method);
std::ostream& operator<<(std::ostream&, http_version);
std::ostream& operator<<(std::ostream&, http_status_line const& status_line);
std::ostream& operator<<(std::ostream&, http_response const& response);

#endif // HTTP_PRINTER_H
