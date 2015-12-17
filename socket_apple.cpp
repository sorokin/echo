#include "socket_apple.h"

#include <sys/socket.h>
#include <errno.h>
#include <netinet/ip.h>
#include <fcntl.h>

#include "throw_error.h"

namespace
{
    int get_fd_flags(int fd);
    void set_fd_flags(int fd, int flags);
    
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
: client_socket(ep, std::move(fd), std::move(on_disconnect), on_ready_t{}, on_ready_t{})
{}

client_socket::client_socket(epoll& ep,
                             file_descriptor fd,
                             on_ready_t on_disconnect,
                             on_ready_t on_read_ready,
                             on_ready_t on_write_ready)
: pimpl(new impl(ep, std::move(fd), std::move(on_disconnect), std::move(on_read_ready), std::move(on_write_ready)))
{}

client_socket::impl::impl(sysapi::epoll &ep, file_descriptor fd, on_ready_t on_disconnect, on_ready_t on_read_ready, on_ready_t on_write_ready)
    : ep(ep)
    , fd(std::move(fd))
    , on_disconnect(std::move(on_disconnect))
    , on_read_ready(std::move(on_read_ready))
    , on_write_ready(std::move(on_write_ready))
    , reg(ep, this->fd.getfd(), {EVFILT_READ}, [this](struct kevent event)
        {
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
    , destroyed(nullptr)
{}

client_socket::impl::~impl()
{
    if (destroyed)
        *destroyed = true;
}

void client_socket::impl::update_registration()
{
    reg.modify(calculate_flags());
}

std::list<int16_t> client_socket::impl::calculate_flags() const
{
    std::list<int16_t> ev_list;
    if (on_read_ready)
        ev_list.push_back(EVFILT_READ);
    if (on_write_ready)
        ev_list.push_back(EVFILT_WRITE);
    
    return ev_list;
}

void client_socket::set_on_read_write(on_ready_t on_read_ready,
                                      on_ready_t on_write_ready)
{
    pimpl->on_read_ready = std::move(on_read_ready);
    pimpl->on_write_ready = std::move(on_write_ready);
    pimpl->update_registration();
}

void client_socket::set_on_read(on_ready_t on_ready)
{
    // TODO: not exception safe
    pimpl->on_read_ready = on_ready;
    pimpl->update_registration();
}

void client_socket::set_on_write(client_socket::on_ready_t on_ready)
{
    pimpl->on_write_ready = on_ready;
    pimpl->update_registration();
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
    file_descriptor fd = make_socket(AF_INET, SOCK_STREAM);
    connect_socket(fd.getfd(), remote.port_net, remote.addr_net);
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
    client_socket res{ep, std::move(fd), std::move(on_disconnect)};
    return res;
}

server_socket::server_socket(epoll& ep, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM))
    , on_connected(on_connected)
    , reg(ep, fd.getfd(), {EVFILT_READ}, [this](struct kevent event) {
        assert(event.filter == EVFILT_READ);
        this->on_connected();
    })
{
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
    start_listen(fd.getfd());
}

server_socket::server_socket(epoll& ep, ipv4_endpoint local_endpoint, on_connected_t on_connected)
    : fd(make_socket(AF_INET, SOCK_STREAM))
    , on_connected(on_connected)
    , reg(ep, fd.getfd(), {EVFILT_READ}, [this](struct kevent event) {
        assert(event.filter & EVFILT_READ);
        this->on_connected();
    })
{
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
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
    int res = ::accept(fd.getfd(), nullptr, nullptr);
    if (res == -1)
        throw_error(errno, "accept()");
    
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
  
    const int set = 1;
    ::setsockopt(res, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));  // NOSIGPIPE FOR SEND
    
    return client_socket{reg.get_epoll(), {res}, std::move(on_disconnect)};
}

client_socket server_socket::accept(client_socket::on_ready_t on_disconnect,
                                    client_socket::on_ready_t on_read_ready,
                                    client_socket::on_ready_t on_write_ready) const
{
    int res = ::accept(fd.getfd(), nullptr, nullptr);
    if (res == -1)
        throw_error(errno, "accept4()");
    
    set_fd_flags(fd.getfd(), get_fd_flags(fd.getfd()) | O_NONBLOCK);
    
    const int set = 1;
    ::setsockopt(res, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));  // NOSIGPIPE FOR SEND
    
    return client_socket{reg.get_epoll(), {res}, std::move(on_disconnect), std::move(on_read_ready), std::move(on_write_ready)};
}
