#ifndef ASYNC_RESOLVER_H
#define ASYNC_RESOLVER_H

#include "epoll.h"
#include "address.h"
#include "socket.h"
#include "call_once_signal.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <atomic>
#include <experimental/optional>

struct async_resolver
{
    typedef std::vector<ipv4_address> resolution_results;
    typedef void callback_sig(resolution_results const& result);
    typedef std::function<callback_sig> on_resolution_done_t;

    struct resolution_request
    {
        ~resolution_request();
    };

    async_resolver(epoll& ep);

    resolution_request resolve(std::string hostname, on_resolution_done_t on_done);

private:
    std::experimental::optional<std::string> dequeue_request();

private:
    eventfd efd;
    std::mutex m;
    std::condition_variable cv;
    std::map<std::string, call_once_signal<callback_sig> > request_callbacks;
    std::deque<std::string> pending_requests;
    std::deque<std::pair<std::string, resolution_results> > finished_requests;
    std::atomic<bool> finished;
    std::thread worker_thread;
};

#endif // ASYNC_RESOLVER_H
