#include "http_server.h"
#include <iterator>

namespace
{
    constexpr const timer::clock_t::duration timeout = std::chrono::seconds(15);
}

http_server::browser_connection::browser_connection(http_server* parent)
    : parent(parent)
    , socket(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }, [this] {
		size_t received_now = socket.read_some(request_buffer, sizeof request_buffer - request_received);
	}, client_socket::on_ready_t{}))
    , timer(parent->ep, [this] {
        this->parent->connections.erase(this);
    })
	, request_received(0)
{}
