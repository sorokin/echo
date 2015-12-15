#include "cap_read.h"

#include <unistd.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>
#include <vector>

#include "throw_error.h"

size_t read_some(weak_file_descriptor fdc, void* data, size_t size)
{
    int fd = fdc.getfd();

    assert(fd != -1);
    ssize_t res = ::read(fd, data, size);
    if (res == -1)
    {
        int err = errno;
        if (err == EAGAIN)
            return 0;
        throw_error(err, "read()");
    }

    assert(res >= 0);
    size_t bytes_read = static_cast<size_t>(res);
    assert(bytes_read <= size);

    return bytes_read;
}
