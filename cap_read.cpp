#include "cap_read.h"

#include <unistd.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>
#include <vector>

#include "throw_error.h"

void read(weak_file_descriptor fdc, void* data, size_t size)
{
    size_t bytes_read = read_some(fdc, data, size);
    if (bytes_read != size)
    {
        std::stringstream ss;
        ss << "incomplete read, requested: " << size << ", read: " << bytes_read;
        throw std::runtime_error(ss.str());
    }
}

std::string read_string(weak_file_descriptor fd, std::size_t size)
{
    // yes, it is slow, but it is the fastest standard compliant way I know to read data into a string
    char buff[size];
    read(fd, buff, size);
    return std::string{buff, size};
}

size_t read_some(weak_file_descriptor fdc, void* data, size_t size)
{
    int fd = fdc.getfd();

    assert(fd != -1);
    ssize_t res = ::read(fd, data, size);
    if (res == -1)
        throw_error(errno, "read()");

    assert(res >= 0);
    size_t bytes_read = static_cast<size_t>(res);
    assert(bytes_read <= size);

    return bytes_read;
}
