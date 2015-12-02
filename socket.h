#ifndef SOCKET_H
#define SOCKET_H

#include "file_descriptor.h"
#include "address.h"
#include "epoll.h"
#include <memory>
#include <cstdint>

struct client_socket
{
    typedef std::function<void ()> on_ready_t;

    client_socket(epoll& ep,
                  file_descriptor fd,
                  on_ready_t on_disconnect);

    client_socket(epoll& ep,
                  file_descriptor fd,
                  on_ready_t on_disconnect,
                  on_ready_t on_read_ready,
                  on_ready_t on_write_ready);

    void set_on_read_write(on_ready_t on_read_ready, on_ready_t on_write_ready);
    void set_on_read(on_ready_t on_ready);
    void set_on_write(on_ready_t on_ready);

    size_t write_some(void const* data, size_t size);
    size_t read_some(void* data, size_t size);

    static client_socket connect(epoll& ep, ipv4_endpoint const& remote, on_ready_t on_disconnect);

private:
    struct impl
    {
        impl(epoll& ep, file_descriptor fd, on_ready_t on_disconnect, on_ready_t on_read_ready, on_ready_t on_write_ready);
        ~impl();

        void update_registration();
        int calculate_flags() const;

        epoll& ep;
        file_descriptor fd;
        on_ready_t on_disconnect;
        on_ready_t on_read_ready;
        on_ready_t on_write_ready;
        epoll_registration reg;
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
    client_socket accept(client_socket::on_ready_t on_disconnect,
                         client_socket::on_ready_t on_read_ready,
                         client_socket::on_ready_t on_write_ready) const;

private:
    file_descriptor fd;
    on_connected_t on_connected;
    epoll_registration reg;
};

struct eventfd
{
    typedef std::function<void ()> on_event_t;

    eventfd(epoll& ep, bool semaphore, on_event_t on_event);
    void notify(uint64_t increment = 1);
    void set_on_event(on_event_t on_event);

private:
    file_descriptor fd;
    on_event_t on_event;
    epoll_registration reg;
};

#endif // SOCKET_H
