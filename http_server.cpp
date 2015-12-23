#include "http_server.h"

#include "http_parser.h"

#include <algorithm>
#include <iterator>

namespace
{
    constexpr const timer::clock_t::duration timeout = std::chrono::seconds(15);
    char crlf_crlf[4] = {'\r', '\n', '\r', '\n'};
}

http_server::browser_connection::browser_connection(http_server* parent)
    : parent(parent)
    , socket(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }, [this] {
        size_t received_now = socket.read_some(request_buffer, sizeof request_buffer - request_received);
        if (received_now == 0)
            return;

        auto i = std::search(request_buffer + request_received - 3,
                             request_buffer + request_received + received_now,
                             std::begin(crlf_crlf),
                             std::end(crlf_crlf));

        if (i == request_buffer + request_received + received_now)
            return;

        socket.set_on_read(client_socket::on_ready_t{});

        new_request(request_buffer, i);

    }, client_socket::on_ready_t{}))
    , timer(parent->ep, [this] {
        this->parent->connections.erase(this);
    })
    , request_received(0)
{}

void http_server::browser_connection::new_request(char const* begin, char const* end)
{
    sub_string text(begin, end);
    http_request request = parse_request(text);

    if (request.request_line.method != http_request_method::GET
     && request.request_line.method != http_request_method::HEAD)
    {
        // rfc2616 [5.1.1]
        // An origin server SHOULD return the status code 405 (Method Not
        // Allowed) if the method is known by the origin server but not
        // allowed for the requested resource, and 501 (Not Implemented) if
        // the method is unrecognized or not implemented by the origin
        // server. The methods GET and HEAD MUST be supported by all general
        // purpose servers.

        http_response responce;
        responce.status_line.version = http_version::HTTP_10;
        responce.status_line.status_code = 501;
        responce.status_line.reason_phrase = sub_string::literal("Not Implemented");

        std::stringstream 
    }

    target.reset(client_socket::connect(parent->ep, ))
}
