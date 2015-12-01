#include "kqueue.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "throw_error.h"

using namespace sysapi;

epoll::epoll()
{
    int r = kqueue();
    
    if (r == -1)
        throw_error(errno, "kqueue()");
    
    assert(r >= 0);
    
    fd_ = r;
}

epoll::epoll(epoll&& rhs)
    : fd_(rhs.fd_)
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
        assert(r <= ev_size);
        
        for (size_t i = 0; i < r; ++i)
        {
            try
            {
                struct kevent const& ee = ev[i];
                if (ev[i].ident == -1) {
                    continue;
                }
                static_cast<epoll_registration*>(ee.udata)->callback(ee);
                if (event_deleted) {
                    event_deleted = false;
                    for (size_t j = i + 1; j < r; j++) {
                        if (ev[j].ident == ev[i].ident) {
                            ev[j].ident = -1;
                        }
                    }
                }
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
    }
}

void epoll::add(int fd, int16_t events, epoll_registration* reg)
{
    struct kevent event;
    EV_SET(&event, fd, events, EV_ADD, 0, 0, reg);
    
    int r = kevent(fd_, &event, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_ADD)");
}

void epoll::modify(int fd, std::list<int16_t> events, epoll_registration* reg)
{
    for (auto it = events.begin(); it != events.end(); it++) {
        struct kevent event;
        EV_SET(&event, fd, *it, EV_DELETE, 0, 0, reg);
        kevent(fd_, &event, 1, NULL, 0, NULL);
        
        EV_SET(&event, fd, *it, EV_ADD, 0, 0, reg);
        
        int r = kevent(fd_, &event, 1, NULL, 0, NULL);
        if (r < 0)
            throw_error(errno, "kevent() MOD");
    }
    event_deleted = true;
}

void epoll::remove(int fd, int16_t events)
{
    struct kevent event;
    EV_SET(&event, fd, events, EV_DELETE, 0, 0, NULL);
    
    int r = kevent(fd_, &event, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_DELETE)");
    
    event_deleted = true;
}

epoll_registration::epoll_registration()
: ep()
, fd(-1)
, events()
{}

epoll_registration::epoll_registration(epoll& ep, int fd, std::list<int16_t> events, callback_t callback)
: ep(&ep)
, fd(fd)
, events(events)
, callback(std::move(callback))
{
    for (auto it = events.begin(); it != events.end(); it++) {
        ep.add(fd, *it, this);
    }
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
    rhs.events = events;
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

void epoll_registration::modify(std::list<int16_t> new_events)
{
    assert(ep);
    
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
        for (auto it = events.begin(); it != events.end(); it++) {
            ep->remove(fd, *it);
        }
        ep = nullptr;
        fd = -1;
        events = {};
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