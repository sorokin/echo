#include "socket.h"

#include <sys/socket.h>
#include <errno.h>
#include <netinet/ip.h>

#include "throw_error.h"
#ifndef __APPLE__
#include <sys/epoll.h>
#include <sys/eventfd.h>
#endif
#include "cap_write.h"
#include "cap_read.h"

namespace
{
    file_descriptor make_socket(int domain, int type)
    {
        int fd = ::socket(domain, type, 0);
        if (fd == -1)
            throw_error(errno, "socket()");

        return file_descriptor{fd};
    }

    void start_listen(int fd)
    {
        int res = ::listen(fd, SOMAXCONN);
        if (res == -1)
            throw_error(errno, "listen()");
    }

    void bind_socket(int fd, uint16_t port_net, uint32_t addr_net)
    {
        sockaddr_in saddr{};
        saddr.sin_family = AF_INET;
        saddr.sin_port = port_net;
        saddr.sin_addr.s_addr = addr_net;
        int res = ::bind(fd, reinterpret_cast<sockaddr const*>(&saddr), sizeof saddr);
        if (res == -1)
            throw_error(errno, "bind()");
    }

    void connect_socket(int fd, uint16_t port_net, uint32_t addr_net)
    {
        sockaddr_in saddr{};
        saddr.sin_family = AF_INET;
        saddr.sin_port = port_net;
        saddr.sin_addr.s_addr = addr_net;

        int res = ::connect(fd, reinterpret_cast<sockaddr const*>(&saddr), sizeof saddr);
        if (res == -1)
            throw_error(errno, "connect()");
    }
}

client_socket::client_socket(sysapi::epoll &ep, file_descriptor fd, on_ready_t on_disconnect)
    : pimpl(new impl(ep, std::move(fd), std::move(on_disconnect)))
{}

client_socket::impl::impl(sysapi::epoll &ep, file_descriptor fd, on_ready_t on_disconnect)
    : ep(ep)
    , fd(std::move(fd))
#ifdef __APPLE__
    , reg(ep, this->fd.getfd(), EVFILT_READ, [this](struct kevent event) {
        assert(event.filter & EVFILT_READ);
#else
    , reg(ep, this->fd.getfd(), 0, [this](uint32_t events) {
        assert((events & ~(EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP)) == 0);
#endif
        bool is_destroyed = false;
        assert(destroyed == nullptr);
        destroyed = &is_destroyed;
        try
        {
#ifdef __APPLE__
            if (event.filter & EVFILT_READ && event.flags & EV_EOF && event.data == 0)
            {
                this->on_disconnect();
                if (is_destroyed)
                    return;
            }
            if (event.filter & EVFILT_READ)
            {
                this->on_read_ready();
                if (is_destroyed)
                    return;
            }
        
#else
            if ((events & EPOLLRDHUP)
                || (events & EPOLLERR)
                || (events & EPOLLHUP))
            {
                this->on_disconnect();
                if (is_destroyed)
                    return;
            }
            if (events & EPOLLIN)
            {
                this->on_read_ready();
                if (is_destroyed)
                    return;
            }
            if (events & EPOLLOUT)
            {
                this->on_write_ready();
                if (is_destroyed)
                    return;
            }
#endif
        }
        catch (...)
        {
            destroyed = nullptr;
            throw;
        }
        destroyed = nullptr;
    })
    , on_disconnect(std::move(on_disconnect))
    , destroyed(nullptr)
{}

client_socket::impl::~impl()
{
    if (destroyed)
        *destroyed = true;
}

void client_socket::set_on_read(on_ready_t on_ready)
{
    // TODO: not exception safe
    pimpl->on_read_ready = on_ready;
    update_registration();
}
#ifndef __APPLE__
void client_socket::set_on_write(client_socket::on_ready_t on_ready)
{
    pimpl->on_write_ready = on_ready;
    update_registration();
}
#endif
size_t client_socket::write_some(const void *data, size_t size)
{
    return ::write_some(pimpl->fd, data, size);
}

size_t client_socket::read_some(void* data, size_t size)
{
    return ::read_some(pimpl->fd, data, size);
}

client_socket client_socket::connect(sysapi::epoll &ep, const ipv4_endpoint &remote, on_ready_t on_disconnect)
{
    file_descriptor fd = make_socket(AF_INET, SOCK_STREAM);
    connect_socket(fd.getfd(), remote.port_net, remote.addr_net);
    client_socket res{ep, std::move(fd), std::move(on_disconnect)};
    return res;
}

void client_socket::update_registration()
{
#ifdef __APPLE__
    pimpl->reg.modify(EVFILT_READ);
#else
    pimpl->reg.modify((pimpl->on_read_ready ? EPOLLIN : 0)
                      | (pimpl->on_write_ready ? EPOLLOUT: 0)
                      | EPOLLRDHUP);
#endif
}

server_socket::server_socket(epoll& ep, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM))
    , on_connected(on_connected)
#ifdef __APPLE__
    , reg(ep, fd.getfd(), EVFILT_READ, [this](struct kevent event) {
        assert(event.filter & EVFILT_READ);
#else
    , reg(ep, fd.getfd(), EPOLLIN, [this](uint32_t events) {
        assert(events == EPOLLIN);
#endif
        this->on_connected();
    })
{
    start_listen(fd.getfd());
}

server_socket::server_socket(epoll& ep, ipv4_endpoint local_endpoint, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM))
    , on_connected(on_connected)
#ifdef __APPLE__
        , reg(ep, fd.getfd(), EVFILT_READ, [this](struct kevent event) {
            assert(event.filter & EVFILT_READ);
#else
        , reg(ep, fd.getfd(), EPOLLIN, [this](uint32_t events) {
            assert(events == EPOLLIN);
#endif
        this->on_connected();
    })
{
    bind_socket(fd.getfd(), local_endpoint.port_net, local_endpoint.addr_net);
    start_listen(fd.getfd());
}

ipv4_endpoint server_socket::local_endpoint() const
{
    sockaddr_in saddr{};
    socklen_t saddr_len = sizeof saddr;
    int res = ::getsockname(fd.getfd(), reinterpret_cast<sockaddr*>(&saddr), &saddr_len);
    if (res == -1)
        throw_error(errno, "getsockname()");
    assert(saddr_len == sizeof saddr);
    return ipv4_endpoint{saddr.sin_port, saddr.sin_addr.s_addr};
}

client_socket server_socket::accept(client_socket::on_ready_t on_disconnect) const
{
#ifdef __APPLE__
    struct sockaddr addr;
    socklen_t socklen = sizeof(addr);
    int res = ::accept(fd.getfd(), &addr, &socklen);
    if (res == -1)
        throw_error(errno, "accept()");
  
    const int set = 1;
    ::setsockopt(res, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));  // NOSIGPIPE FOR SEND
    
#else
    int res = ::accept4(fd.getfd(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (res == -1)
        throw_error(errno, "accept4()");

#endif
    return client_socket{reg.get_epoll(), {res}, std::move(on_disconnect)};
}
