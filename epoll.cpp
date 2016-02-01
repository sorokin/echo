#include "epoll.h"

#include <sys/epoll.h>

#include <array>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "throw_error.h"

using namespace sysapi;

epoll::epoll()
{
    int r = ::epoll_create1(EPOLL_CLOEXEC);
    if (r == -1)
        throw_error(errno, "epoll_create1()");

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
        std::array<epoll_event, 100> ev;

    again:
        int timeout = run_timers_calculate_timeout();
        int r = ::epoll_wait(fd_.getfd(), ev.data(), ev.size(), timeout);

        if (r < 0)
        {
            int err = errno;

            if (err == EINTR)
                goto again;

            throw_error(err, "epoll_wait()");
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
    }
}

timer& epoll::get_timer()
{
    return timer_;
}

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
