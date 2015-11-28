#ifndef CAP_READ_H
#define CAP_READ_H

#include "file_descriptor.h"

#include <array>
#include <string>

void read(weak_file_descriptor fd, void* data, size_t size);
std::string read_string(weak_file_descriptor fd, std::size_t size);

template <size_t N>
std::array<char, N> read_array(weak_file_descriptor fd)
{
    std::array<char, N> res;
    read(fd, res.data(), res.size());
    return res;
}

size_t read_some(weak_file_descriptor fd, void* data, size_t size);

#endif // CAP_READ_H
