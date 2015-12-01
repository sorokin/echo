#include "socket_apple.h"

#include <sys/socket.h>
#include <errno.h>
#include <netinet/ip.h>
#include <fcntl.h>

#include "throw_error.h"
#include "cap_write.h"
#include "cap_read.h"

namespace
{
    int get_fd_flags(int fd);
    void set_fd_flags(int fd, int flags);
    
    file_descriptor make_socket(int domain, int type, bool nonblock)
    {
        int fd = ::socket(domain, type, 0);
        if (fd == -1)
            throw_error(errno, "socket()");
        
        if (nonblock) {
            set_fd_flags(fd, get_fd_flags(fd) | O_NONBLOCK);
        }
        
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

    int get_fd_flags(int fd)
    {
        int res = fcntl(fd, F_GETFL, 0);
        if (res == -1)
            throw_error(errno, "fcntl(F_GETFL)");
        return res;
    }

    void set_fd_flags(int fd, int flags)
    {
        int res = fcntl(fd, F_SETFL, flags);
        if (res == -1)
            throw_error(errno, "fcntl(F_SETFL)");
    }
}

client_socket::client_socket(sysapi::epoll &ep, file_descriptor fd, on_ready_t on_disconnect)
    : pimpl(new impl(ep, std::move(fd), std::move(on_disconnect)))
{}

client_socket::impl::impl(sysapi::epoll &ep, file_descriptor fd, on_ready_t on_disconnect)
    : ep(ep)
    , fd(std::move(fd))
    , reg(ep, this->fd.getfd(), {EVFILT_READ}, [this](struct kevent event) {
        assert(event.filter == EVFILT_READ || event.filter == EVFILT_WRITE);
        bool is_destroyed = false;
        assert(destroyed == nullptr);
        destroyed = &is_destroyed;
        try
        {
            if ((event.filter == EVFILT_READ && event.flags & EV_EOF) || (event.filter == EVFILT_WRITE && event.flags & EV_EOF))
            {
                this->on_disconnect();
                if (is_destroyed)
                    return;
            }
            if (event.filter == EVFILT_READ)
            {
                this->on_read_ready();
                if (is_destroyed)
                    return;
            }
            if (event.filter == EVFILT_WRITE)
            {
                this->on_write_ready();
                if (is_destroyed)
                    return;
            }
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

void client_socket::set_on_write(client_socket::on_ready_t on_ready)
{
    pimpl->on_write_ready = on_ready;
    update_registration();
}

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
    file_descriptor fd = make_socket(AF_INET, SOCK_STREAM, false);
    connect_socket(fd.getfd(), remote.port_net, remote.addr_net);
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
    client_socket res{ep, std::move(fd), std::move(on_disconnect)};
    return res;
}

void client_socket::update_registration()
{
    pimpl->reg.modify({EVFILT_READ, EVFILT_WRITE});
}

server_socket::server_socket(epoll& ep, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM, true))
    , on_connected(on_connected)
    , reg(ep, fd.getfd(), {EVFILT_READ}, [this](struct kevent event) {
        assert(event.filter & EVFILT_READ);
        this->on_connected();
    })
{
    start_listen(fd.getfd());
}

server_socket::server_socket(epoll& ep, ipv4_endpoint local_endpoint, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM, true))
    , on_connected(on_connected)
    , reg(ep, fd.getfd(), {EVFILT_READ}, [this](struct kevent event) {
        assert(event.filter & EVFILT_READ);
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
    struct sockaddr addr;
    socklen_t socklen = sizeof(addr);
    int res = ::accept(fd.getfd(), &addr, &socklen);
    if (res == -1)
        throw_error(errno, "accept()");
  
    const int set = 1;
    ::setsockopt(res, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));  // NOSIGPIPE FOR SEND
    
    return client_socket{reg.get_epoll(), {res}, std::move(on_disconnect)};
}
