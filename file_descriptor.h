#ifndef FILE_DESCIPTOR_H
#define FILE_DESCIPTOR_H

#include <assert.h>
#include <unistd.h>

#include <type_traits>
#include <utility>

struct file_descriptor;

struct weak_file_descriptor
{
    weak_file_descriptor() noexcept;
    explicit weak_file_descriptor(int fd) noexcept;
    weak_file_descriptor(file_descriptor const&) noexcept;
    weak_file_descriptor(weak_file_descriptor const& other) noexcept;
    weak_file_descriptor& operator=(weak_file_descriptor const& other) noexcept;

    void close() noexcept;
    int getfd() const noexcept;

    friend void swap(weak_file_descriptor& a, weak_file_descriptor& b) noexcept;

private:
    int fd;
};

struct file_descriptor : weak_file_descriptor
{
    typedef weak_file_descriptor weak;

    file_descriptor() noexcept;
    file_descriptor(int fd) noexcept;
    file_descriptor(file_descriptor const& other) = delete;
    file_descriptor(file_descriptor&& other) noexcept;
    ~file_descriptor() noexcept;
    file_descriptor& operator=(file_descriptor&& rhs) noexcept;

    weak release() noexcept;
    void reset(int new_fd) noexcept;

    friend void swap(file_descriptor& a, file_descriptor& b) noexcept;
};

#endif // FILE_DESCIPTOR_H
