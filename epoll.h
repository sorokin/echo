#ifndef EPOLL_H
#define EPOLL_H

#include "file_descriptor.h"

#include <functional>
#include <cstdint>

namespace sysapi
{
    struct epoll;
    struct epoll_registration;

    struct epoll
    {
        typedef std::function<void ()> action_t;
        epoll();
        epoll(epoll const&) = delete;
        epoll(epoll&&);

        epoll& operator=(epoll);

        void swap(epoll& other);

        void run();

    private:
        void add(int fd, uint32_t events, epoll_registration*);
        void modify(int fd, uint32_t events, epoll_registration*);
        void remove(int fd);

    private:
        file_descriptor fd_;

        friend struct epoll_registration;
    };

    struct epoll_registration
    {
        typedef std::function<void (uint32_t)> callback_t;

        epoll_registration();
        epoll_registration(epoll&, int fd, uint32_t events, callback_t callback);
        epoll_registration(epoll_registration const&) = delete;
        epoll_registration(epoll_registration&&);
        ~epoll_registration();

        epoll_registration& operator=(epoll_registration);

        void modify(uint32_t new_events);

        void swap(epoll_registration& other);

        void clear();
        epoll& get_epoll() const;

    private:
        void update();

    private:
        epoll* ep;
        int fd;
        uint32_t events;
        callback_t callback;

        friend struct epoll;
    };
}

using sysapi::epoll;
using sysapi::epoll_registration;

#endif