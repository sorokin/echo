#include "cap_write.h"

#include "throw_error.h"
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>

size_t write_some(weak_file_descriptor fdc, void const* data, std::size_t size)
{
    int fd = fdc.getfd();

    assert(fd != -1);
    ssize_t res = ::send(fd, data, size, MSG_NOSIGNAL);
    if (res == -1)
    {
        int err = errno;
        if (err == EAGAIN || err == ECONNRESET || err == EPIPE)
            return 0;
        throw_error(err, "write()");
    }

    assert(res >= 0);
    size_t written = static_cast<size_t>(res);
    assert(written <= size);

    return written;
}

void write(weak_file_descriptor fdc, const void *data, std::size_t size)
{
    size_t written = write_some(fdc, data, size);

    if (written != size)
    {
        std::stringstream ss;
        ss << "incomplete write, requested: " << size << ", written: " << written;
        throw std::runtime_error(ss.str());
    }
}
