#ifndef SOCKET_APPLE_H
#define SOCKET_APPLE_H

#include "file_descriptor.h"
#include "address.h"
#include "kqueue.hpp"
#include <memory>
#include <cstdint>

struct client_socket
{
    typedef std::function<void ()> on_ready_t;

    client_socket(epoll& ep, file_descriptor fd, on_ready_t on_disconnect);

    void set_on_read(on_ready_t on_ready);
    void set_on_write(on_ready_t on_ready);

    size_t write_some(void const* data, size_t size);
    size_t read_some(void* data, size_t size);

    static client_socket connect(epoll& ep, ipv4_endpoint const& remote, on_ready_t on_disconnect);

private:
    void update_registration();

private:
    struct impl
    {
        impl(epoll& ep, file_descriptor fd, on_ready_t on_disconnect);
        ~impl();

        epoll& ep;
        file_descriptor fd;
        epoll_registration reg;
        on_ready_t on_disconnect;
        on_ready_t on_read_ready;
        on_ready_t on_write_ready;
        bool* destroyed;
    };

    std::unique_ptr<impl> pimpl;
};

struct server_socket
{
    typedef std::function<void ()> on_connected_t;

    server_socket(epoll& ep, on_connected_t on_connected);
    server_socket(epoll& ep, ipv4_endpoint local_endpoint, on_connected_t on_connected);

    ipv4_endpoint local_endpoint() const;
    client_socket accept(client_socket::on_ready_t on_disconnect) const;

private:
    file_descriptor fd;
    on_connected_t on_connected;
    epoll_registration reg;
};

#endif // SOCKET_APPLE_H
