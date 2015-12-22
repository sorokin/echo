#include "http_server.h"

namespace
{
    constexpr const timer::clock_t::duration timeout = std::chrono::seconds(15);
}

http_server::connection::connection(http_server* parent)
    : parent(parent)
    , browser(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }))
    , timer(parent->ep, [this] {
        this->parent->connections.erase(this);
    })
{}
