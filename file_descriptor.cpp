#include "file_descriptor.h"

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
