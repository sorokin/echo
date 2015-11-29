#include "epoll.h"

#ifndef __APPLE__
#include <sys/epoll.h>

#include <array>
#endif

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "throw_error.h"

using namespace sysapi;

epoll::epoll()
{
#ifdef __APPLE__
    int r = kqueue();
    
    if (r == -1)
        throw_error(errno, "kqueue()");

    assert(r >= 0);

    fd_ = r;
#else 
    int r = ::epoll_create1(EPOLL_CLOEXEC);
    if (r == -1)
        throw_error(errno, "epoll_create1()");
    
    assert(r >= 0);
    
    fd_.reset(r);
#endif
}

epoll::epoll(epoll&& rhs)
#ifdef __APPLE__
    : fd_(rhs.fd_)
#else
    : fd_(std::move(rhs.fd_))
#endif
{}

epoll& epoll::operator=(epoll rhs)
{
    swap(rhs);
    return *this;
}

void epoll::swap(epoll& other)
{
    using std::swap;
    swap(fd_, other.fd_);
}

void epoll::run()
{
    for (;;)
    {
#ifdef __APPLE__
        int const ev_size = 16;
        struct kevent ev[ev_size];
        
        int r = kevent(fd_, NULL, 0, ev, ev_size, NULL);

        if (r < 0)
        {
            int err = errno;

            if (err == EINTR)
                continue;

            throw_error(err, "kevent()");
        }

        assert(r > 0);
        size_t num_events = static_cast<size_t>(r);
        assert(num_events <= ev_size);

        for (size_t i = 0; i < r; ++i)
        {
            try
            {
                struct kevent const& ee = ev[i];
                static_cast<epoll_registration*>(ee.udata)->callback(ee);
            }
            catch (std::exception const& e)
            {
                std::cerr << "error: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "unknown exception in message loop" << std::endl;
            }
        }
#else
        std::array<epoll_event, 16> ev;
        
    again:
        int r = ::epoll_wait(fd_.getfd(), ev.data(), ev.size(), -1);
        
        if (r < 0)
        {
            int err = errno;
            
            if (err == EINTR)
                goto again;
            
            throw_error(err, "epoll_wait()");
        }
        
        assert(r > 0);
        size_t num_events = static_cast<size_t>(r);
        assert(num_events <= ev.size());
        
        for (auto i = ev.begin(); i != ev.begin() + num_events; ++i)
        {
            try
            {
                epoll_event const& ee = *i;
                static_cast<epoll_registration*>(ee.data.ptr)->callback(ee.events);
            }
            catch (std::exception const& e)
            {
                std::cerr << "error: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "unknown exception in message loop" << std::endl;
            }
        }
#endif
    }
}

#ifdef __APPLE__
void epoll::add(int fd, int16_t events, epoll_registration* reg)
{
    struct kevent event;
    EV_SET(&event, fd, events, EV_ADD, 0, 0, reg);

    int r = kevent(fd_, &event, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_ADD)");
}

void epoll::modify(int fd, int16_t events, epoll_registration* reg)
{
    struct kevent event;
    EV_SET(&event, fd, events, EV_DELETE, 0, 0, reg);
    kevent(fd_, &event, 1, NULL, 0, NULL);
    
    EV_SET(&event, fd, events, EV_ADD, 0, 0, reg);
    
    int r = kevent(fd_, &event, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent() MOD");
}

void epoll::remove(int fd, int16_t events)
{
    struct kevent event;
    EV_SET(&event, fd, events, EV_DELETE, 0, 0, NULL);
    
    int r = kevent(fd_, &event, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_DELETE)");
}

epoll_registration::epoll_registration()
    : ep()
    , fd(-1)
    , events()
{}

epoll_registration::epoll_registration(epoll& ep, int fd, uint32_t events, callback_t callback)
    : ep(&ep)
    , fd(fd)
    , events(events)
    , callback(std::move(callback))
{
    ep.add(fd, events, this);
}

epoll_registration::epoll_registration(epoll_registration&& rhs)
    : ep(rhs.ep)
    , fd(rhs.fd)
    , events(rhs.events)
    , callback(std::move(rhs.callback))
{
    update();
    rhs.ep = nullptr;
    rhs.fd = -1;
    rhs.events = 0;
    rhs.callback = callback_t();
}

epoll_registration::~epoll_registration()
{
    clear();
}

epoll_registration& epoll_registration::operator=(epoll_registration rhs)
{
    swap(rhs);
    return *this;
}

void epoll_registration::modify(int16_t new_events)
{
    assert(ep);

    if (events == new_events)
        return;

    ep->modify(fd, new_events, this);
    events = new_events;
}

void epoll_registration::swap(epoll_registration& other)
{
    std::swap(ep,       other.ep);
    std::swap(fd,       other.fd);
    std::swap(events,   other.events);
    std::swap(callback, other.callback);
    update();
    other.update();
}

void epoll_registration::clear()
{
    if (ep)
    {
        ep->remove(fd, events);
        ep = nullptr;
        fd = -1;
        events = 0;
    }
}

epoll& epoll_registration::get_epoll() const
{
    return *ep;
}

void epoll_registration::update()
{
    if (ep)
        ep->modify(fd, events, this);
}

#else 

void epoll::add(int fd, uint32_t events, epoll_registration* reg)
{
    epoll_event ev = {0, 0};
    ev.data.ptr = reg;
    ev.events   = events;
    
    int r = ::epoll_ctl(fd_.getfd(), EPOLL_CTL_ADD, fd, &ev);
    if (r < 0)
        throw_error(errno, "epoll_ctl(EPOLL_CTL_ADD)");
}

void epoll::modify(int fd, uint32_t events, epoll_registration* reg)
{
    epoll_event ev = {0, 0};
    ev.data.ptr = reg;
    ev.events   = events;
    
    int r = ::epoll_ctl(fd_.getfd(), EPOLL_CTL_MOD, fd, &ev);
    if (r < 0)
        throw_error(errno, "epoll_ctl(EPOLL_CTL_MOD)");
}

void epoll::remove(int fd)
{
    int r = ::epoll_ctl(fd_.getfd(), EPOLL_CTL_DEL, fd, nullptr);
    if (r < 0)
        throw_error(errno, "epoll_ctl(EPOLL_CTL_DEL)");
}

epoll_registration::epoll_registration()
    : ep()
    , fd(-1)
    , events()
{}

epoll_registration::epoll_registration(epoll& ep, int fd, uint32_t events, callback_t callback)
    : ep(&ep)
    , fd(fd)
    , events(events)
    , callback(std::move(callback))
{
    ep.add(fd, events, this);
}

epoll_registration::epoll_registration(epoll_registration&& rhs)
    : ep(rhs.ep)
    , fd(rhs.fd)
    , events(rhs.events)
    , callback(std::move(rhs.callback))
{
    update();
    rhs.ep = nullptr;
    rhs.fd = -1;
    rhs.events = 0;
    rhs.callback = callback_t();
}

epoll_registration::~epoll_registration()
{
    clear();
}

epoll_registration& epoll_registration::operator=(epoll_registration rhs)
{
    swap(rhs);
    return *this;
}

void epoll_registration::modify(uint32_t new_events)
{
    assert(ep);
    
    if (events == new_events)
        return;
    
    ep->modify(fd, new_events, this);
    events = new_events;
}

void epoll_registration::swap(epoll_registration& other)
{
    std::swap(ep,       other.ep);
    std::swap(fd,       other.fd);
    std::swap(events,   other.events);
    std::swap(callback, other.callback);
    update();
    other.update();
}

void epoll_registration::clear()
{
    if (ep)
    {
        ep->remove(fd);
        ep = nullptr;
        fd = -1;
        events = 0;
    }
}

epoll& epoll_registration::get_epoll() const
{
    return *ep;
}

void epoll_registration::update()
{
    if (ep)
        ep->modify(fd, events, this);
}
#endif
