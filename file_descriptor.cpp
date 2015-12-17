#include "file_descriptor.h"

#include <sstream>
#include <stdexcept>
#include <errno.h>
#include <sys/socket.h>
#include "throw_error.h"

weak_file_descriptor::weak_file_descriptor() noexcept
    : fd(-1)
{}

weak_file_descriptor::weak_file_descriptor(int fd) noexcept
    : fd(fd)
{}

weak_file_descriptor::weak_file_descriptor(file_descriptor const& handle) noexcept
    : fd(handle.getfd())
{}

weak_file_descriptor::weak_file_descriptor(weak_file_descriptor const& other) noexcept
    : fd(other.fd)
{}

weak_file_descriptor& weak_file_descriptor::operator=(weak_file_descriptor const& other) noexcept
{
    fd = other.getfd();
    return *this;
}

void weak_file_descriptor::close() noexcept
{
    if (fd == -1)
        return;

    int r = ::close(fd);
    assert(r == 0);

    fd = -1;
}

int weak_file_descriptor::getfd() const noexcept
{
    return fd;
}

void swap(weak_file_descriptor& a, weak_file_descriptor& b) noexcept
{
    std::swap(a.fd, b.fd);
}

file_descriptor::file_descriptor() noexcept
{}

file_descriptor::file_descriptor(int fd) noexcept
    : weak(fd)
{}

file_descriptor::file_descriptor(file_descriptor&& other) noexcept
    : weak(other.release())
{}

file_descriptor::~file_descriptor() noexcept
{
    this->close();
}

file_descriptor& file_descriptor::operator=(file_descriptor&& rhs) noexcept
{
    weak& that = *this;

    that = rhs.release();
    return *this;
}

file_descriptor::weak file_descriptor::release() noexcept
{
    weak& that = *this;

    weak tmp = that;
    that = weak{};
    return tmp;
}

void file_descriptor::reset(int new_fd) noexcept
{
    weak& that = *this;

    this->close();
    that = weak{new_fd};
}

void swap(file_descriptor& a, file_descriptor& b) noexcept
{
    swap(static_cast<weak_file_descriptor&>(a), static_cast<weak_file_descriptor&>(b));
}

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

size_t write_some(weak_file_descriptor fdc, void const* data, std::size_t size)
{
    int fd = fdc.getfd();

    assert(fd != -1);
    ssize_t res = ::send(fd, data, size, 0);
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
