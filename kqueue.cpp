#include "kqueue.hpp"

#include <array>
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
    
    fd_.reset(r);
}

epoll::epoll(epoll&& rhs)
    : fd_(std::move(rhs.fd_))
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
        std::array<struct kevent, 16> ev;

    again:
        int timeout = run_timers_calculate_timeout();
        struct timespec tmout = {timeout, 0};
        int r = kevent(fd_.getfd(), NULL, 0, ev.data(), ev.size(), timeout == -1 ? nullptr : &tmout);
        
        if (r < 0)
        {
            int err = errno;
            
            if (err == EINTR)
                goto again;
            
            throw_error(err, "kevent()");
        }
        
        if (r == 0)
            goto again;
        
        assert(r > 0);
        size_t num_events = static_cast<size_t>(r);
        assert(num_events <= ev.size());
        
        for (auto i = ev.begin(); i != ev.begin() + num_events; ++i)
        {
            try
            {
                struct kevent const& ee = *i;
                if (ee.ident == -1) {
                    continue;
                }
                static_cast<epoll_registration*>(ee.udata)->callback(ee);
                if (!deleted_events.empty()) {
                    for (auto k = deleted_events.begin(); k != deleted_events.end(); k++) {
                        for (auto j = i + 1; j != ev.begin() + num_events; j++) {
                            if (j->ident == k->first && j->filter == k->second) {
                                j->ident = -1;
                            }
                        }
                    }
                    deleted_events = {};
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

timer& epoll::get_timer()
{
    return timer_;
}

void epoll::add(int fd, int16_t event, epoll_registration* reg)
{
    struct kevent ev;
    EV_SET(&ev, fd, event, EV_ADD, 0, 0, reg);
    
    int r = kevent(fd_.getfd(), &ev, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_ADD)");
}

void epoll::modify(int fd, int16_t event, epoll_registration* reg)
{
    struct kevent ev;
    EV_SET(&ev, fd, event, EV_ADD, 0, 0, reg);
        
    int r = kevent(fd_.getfd(), &ev, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent() MOD");
    
    deleted_events.push_back({fd, event});
}

void epoll::remove(int fd, int16_t event)
{
    struct kevent ev;
    EV_SET(&ev, fd, event, EV_DELETE, 0, 0, NULL);
    
    int r = kevent(fd_.getfd(), &ev, 1, NULL, 0, NULL);
    if (r < 0)
        throw_error(errno, "kevent(EV_DELETE)");

    deleted_events.push_back({fd, event});
}

int epoll::run_timers_calculate_timeout()
{
    if (timer_.empty())
        return -1;
    
    timer::clock_t::time_point now = timer::clock_t::now();
    timer_.notify(now);
    
    if (timer_.empty())
        return -1;
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(timer_.top() - now).count();
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
    rhs.events = {};
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
    
    if (events == new_events)
        return;
    
    for (auto it = events.begin(); it != events.end(); it++) {
        ep->modify(fd, *it, this);
    }
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
        for (auto it = events.begin(); it != events.end(); it++) {
            ep->modify(fd, *it, this);
        }
}
