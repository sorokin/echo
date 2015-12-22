#include "async_resolver.h"

/*namespace
{
    async_resolver::resolution_results resolve(std::string hostname)
    {

    }
}*/

async_resolver::async_resolver(sysapi::epoll &ep)
    : efd(ep, false, [this] {
        std::deque<std::pair<std::string, resolution_results> > copy;

        {
            std::lock_guard<std::mutex> lg(m);
            std::swap(copy, finished_requests);
        }

        for (auto const& p : copy)
        {
            auto i = request_callbacks.find(p.first);
            assert(i != request_callbacks.end());

            call_once_signal<callback_sig> const& sig = i->second;

            sig.call_all(p.second);
        }
    })
    , finished(false)
    , worker_thread([this] {
        for (;;)
        {
            std::experimental::optional<std::string> elem = dequeue_request();
            if (!elem)
                break;

            std::string const& selem = *elem;

            //finished_requests.emplace_back(selem, ::resolve(selem));
            efd.notify();
        }
    })
{}

async_resolver::resolution_request async_resolver::resolve(std::string hostname, on_resolution_done_t on_done)
{
    std::lock_guard<std::mutex> lg(m);
    auto& req = request_callbacks[hostname];
    bool was_empty = req.empty();
    req.push_back(on_done);

    if (was_empty)
    {
        pending_requests.push_back(hostname);
        cv.notify_one();
    }
}

std::experimental::optional<std::string> async_resolver::dequeue_request()
{
    std::string tmp;

    {
        std::unique_lock<std::mutex> lg(m);
        for (;;)
        {
            if (finished)
                return std::experimental::nullopt;

            if (pending_requests.empty())
            {
                cv.wait(lg);
                continue;
            }

            tmp = std::move(pending_requests.front());
            pending_requests.pop_front();
        }
    }

    return std::move(tmp);
}
