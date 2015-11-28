#ifndef FILE_DESCIPTOR_H
#define FILE_DESCIPTOR_H

#include <assert.h>
#include <unistd.h>

#include <type_traits>
#include <utility>

struct file_descriptor;

struct weak_file_descriptor
{
    weak_file_descriptor() noexcept
        : fd(-1)
    {}

    explicit weak_file_descriptor(int fd) noexcept
        : fd(fd)
    {}

    weak_file_descriptor(file_descriptor const&) noexcept;

    weak_file_descriptor(weak_file_descriptor const& other) noexcept
        : fd(other.fd)
    {}

    weak_file_descriptor& operator=(weak_file_descriptor const& other) noexcept
    {
        fd = other.getfd();
        return *this;
    }

    void close() noexcept
    {
        if (fd == -1)
            return;

        int r = ::close(fd);
        assert(r == 0);

        fd = -1;
    }

    int getfd() const noexcept
    {
        return fd;
    }

    friend void swap(weak_file_descriptor& a, weak_file_descriptor& b) noexcept
    {
        std::swap(a.fd, b.fd);
    }

private:
    int fd;
};

struct file_descriptor : weak_file_descriptor
{
    typedef weak_file_descriptor weak;

    file_descriptor()
    {}

    file_descriptor(int fd)
        : weak(fd)
    {}

    file_descriptor(file_descriptor const& other) = delete;

    file_descriptor(file_descriptor&& other) noexcept
        : weak(other.release())
    {}

    ~file_descriptor()
    {
        this->close();
    }

    file_descriptor& operator=(file_descriptor&& rhs) noexcept
    {
        weak& that = *this;

        that = rhs.release();
        return *this;
    }

    weak release() noexcept
    {
        weak& that = *this;

        weak tmp = that;
        that = weak{};
        return tmp;
    }

    void reset(int new_fd) noexcept
    {
        weak& that = *this;

        this->close();
        that = weak{new_fd};
    }

    friend void swap(file_descriptor& a, file_descriptor& b) noexcept
    {
        swap(static_cast<weak&>(a), static_cast<weak&>(b));
    }
};

inline weak_file_descriptor::weak_file_descriptor(file_descriptor const& handle) noexcept
    : fd(handle.getfd())
{}

#endif // FILE_DESCIPTOR_H
