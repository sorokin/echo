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

http_server::http_error::http_error(http_status_code status_code, std::string reason_phrase)
    : runtime_error(reason_phrase)
    , status_code(status_code)
    , reason_phrase(std::move(reason_phrase))
{}

http_status_code http_server::http_error::get_status_code() const
{
    return status_code;
}

std::string const& http_server::http_error::get_reason_phrase() const
{
    return reason_phrase;
}

http_server::inbound_connection::inbound_connection(http_server* parent)
    : parent(parent)
    , socket(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }, [this] {
        try_read();
    }, client_socket::on_ready_t{}))
    , timer(parent->ep.get_timer(), timeout, [this] {
        this->parent->connections.erase(this);
    })
    , request_received(0)
    , response_sent(0)
    , response_size(0)
{}

void http_server::inbound_connection::try_read()
{
    size_t received_now = socket.read_some(request_buffer + request_received, sizeof request_buffer - request_received);
    if (received_now == 0)
        return;

    auto i = std::search(request_buffer + request_received - 3,
                         request_buffer + request_received + received_now,
                         std::begin(crlf_crlf),
                         std::end(crlf_crlf));

    request_received += received_now;
    if (i == request_buffer + request_received)
    {
        if (request_received == sizeof request_buffer)
        {
            socket.set_on_read(client_socket::on_ready_t{});
            send_header(http_status_code::bad_request, "Bad Request: request is too large");
        }

        return;
    }

    socket.set_on_read(client_socket::on_ready_t{});

    try
    {
        new_request(request_buffer, i + sizeof crlf_crlf);
    }
    catch (http_error const& e)
    {
        send_header(e.get_status_code(), e.get_reason_phrase());
    }
    catch (std::exception const& e)
    {
        // TODO: if something was already written to socket,
        // we should just drop the connection
        send_header(http_status_code::internal_server_error, e.what());
    }
}

void http_server::inbound_connection::drop()
{
    parent->connections.erase(this);
}

void http_server::inbound_connection::new_request(char const* begin, char const* end)
{
    sub_string text(begin, end);

    http_request request;
    try
    {
        request = parse_request(text);
    }
    catch (http_parsing_error const& e)
    {
        throw http_error(http_status_code::bad_request, e.what());
    }

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

        throw http_error(http_status_code::not_implemented, "Not Implemented");
    }

    auto i = request.headers.find("Host");
    if (i == request.headers.end())
        throw http_error(http_status_code::bad_request, "host header is missing");

    if (i->second.size() != 1)
        throw http_error(http_status_code::bad_request, "multiple host headers");

    std::string const& host = i->second[0];

    http_response response;
    response.status_line.version = http_version::HTTP_10;
    response.status_line.status_code = http_status_code::ok;
    response.status_line.reason_phrase = sub_string::literal("OK");

    std::stringstream ss;
    ss << response;

    if (request.request_line.method == http_request_method::GET)
    {
        for (ipv4_address const& addr : ipv4_address::resolve(host))
        {
            ss << addr << std::endl;
        }
    }

    std::string const& str = ss.str();
    memcpy(response_buffer, str.data(), str.size());
    response_size = str.size();
    try_write();
}

void http_server::inbound_connection::try_write()
{
    response_sent += socket.write_some(response_buffer, response_size - response_sent);

    if (response_sent != response_size)
        socket.set_on_write([this] { try_write(); });
    else
        drop();
}

void http_server::inbound_connection::send_header(http_status_code status_code, std::string const& message)
{
    std::string reason_phrase = status_code_as_string(status_code);
    if (!message.empty())
    {
        reason_phrase += ": ";
        reason_phrase += message;
    }

    http_response response;
    response.status_line.version = http_version::HTTP_10;
    response.status_line.status_code = status_code;
    response.status_line.reason_phrase = sub_string(reason_phrase);

    std::stringstream ss;
    ss << response;
    std::string const& str = ss.str();
    memcpy(response_buffer, str.data(), str.size());
    response_size = str.size();
    try_write();
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
    std::unique_ptr<inbound_connection> cc(new inbound_connection(this));
    inbound_connection* pcc = cc.get();
    connections.emplace(pcc, std::move(cc));
}
