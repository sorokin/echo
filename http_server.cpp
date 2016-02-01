#include "http_server.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <cstring>

#include "http_parser.h"
#include "http_printer.h"

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
        size_t received_now = socket.read_some(request_buffer + request_received, sizeof request_buffer - request_received);
        if (received_now == 0)
            return;

        auto i = std::search(request_buffer + request_received - 3,
                             request_buffer + request_received + received_now,
                             std::begin(crlf_crlf),
                             std::end(crlf_crlf));

        request_received += received_now;
        if (i == request_buffer + request_received)
            return;

        socket.set_on_read(client_socket::on_ready_t{});

        new_request(request_buffer, i + sizeof crlf_crlf);

    }, client_socket::on_ready_t{}))
    , timer(parent->ep.get_timer(), timeout, [this] {
        this->parent->connections.erase(this);
    })
    , request_received(0)
    , response_sent(0)
    , response_size(0)
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

        http_response response;
        response.status_line.version = http_version::HTTP_10;
        response.status_line.status_code = 501;
        response.status_line.reason_phrase = sub_string::literal("Not Implemented");

        std::stringstream ss;
        ss << response;
        std::string const& str = ss.str();
        memcpy(response_buffer, str.data(), str.size());
        response_size = str.size();
        try_write();
    }
    else
    {
        http_response response;
        response.status_line.version = http_version::HTTP_10;
        response.status_line.status_code = 200;
        response.status_line.reason_phrase = sub_string::literal("OK");

        std::stringstream ss;
        ss << response;

        if (request.request_line.method == http_request_method::GET)
            ss << "Hello, world!\n";

        std::string const& str = ss.str();
        memcpy(response_buffer, str.data(), str.size());
        response_size = str.size();
        try_write();
    }

    //target.reset(client_socket::connect(parent->ep, ))
}

void http_server::browser_connection::try_write()
{
    response_sent += socket.write_some(response_buffer, response_size - response_sent);

    if (response_sent != response_size)
        socket.set_on_write([this] { try_write(); });
    else
        parent->connections.erase(this);
}


http_server::http_server(sysapi::epoll &ep)
    : ep(ep)
    , ss{ep, std::bind(&http_server::on_new_connection, this)}
{}

http_server::http_server(sysapi::epoll &ep, const ipv4_endpoint &local_endpoint)
    : ep(ep)
    , ss{ep, local_endpoint, std::bind(&http_server::on_new_connection, this)}
{}

ipv4_endpoint http_server::local_endpoint() const
{
    return ss.local_endpoint();
}

void http_server::on_new_connection()
{
    std::unique_ptr<browser_connection> cc(new browser_connection(this));
    browser_connection* pcc = cc.get();
    connections.emplace(pcc, std::move(cc));
}
