#ifndef CAP_WRITE_H
#define CAP_WRITE_H

#include "file_descriptor.h"
#include <string>

size_t write_some(weak_file_descriptor fd, void const* data, std::size_t size);

void write(weak_file_descriptor fd, void const* data, std::size_t size);
void write(weak_file_descriptor fd, std::string const& str);

template <size_t N>
void write(weak_file_descriptor fd, char (&&literal)[N])
{
    write(literal + 0, N - 1);
}

#endif // CAP_WRITE_H
