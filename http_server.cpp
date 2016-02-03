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

    client_socket try_connect(epoll& ep, std::string const& host, client_socket::on_ready_t on_disconnect)
    {
        for (ipv4_address const& addr : ipv4_address::resolve(host))
        {
            try
            {
                return client_socket::connect(ep, ipv4_endpoint{80, addr}, std::move(on_disconnect));
            }
            catch (std::exception const&)
            {}
        }

        throw std::runtime_error("failed to connect");
    }
}

send_buffer::send_buffer(client_socket& socket)
    : socket(socket)
    , sent()
{}

send_buffer::~send_buffer()
{
    socket.set_on_write(client_socket::on_ready_t{});
}

void send_buffer::append(void const* data, size_t size)
{
    size_t sent_now = 0;
    
    if (buf.empty())
    {
        sent_now = socket.write_some(data, size);
        if (sent_now == size)
            return;
    }

    char const* cdata = static_cast<char const*>(data);
    append_to_buffer(cdata + sent_now, size - sent_now);
}

void send_buffer::set_on_empty(client_socket::on_ready_t on_empty)
{
    this->on_empty = std::move(on_empty);
    if (buf.empty())
        socket.set_on_write(this->on_empty);
}

void send_buffer::append_to_buffer(void const* data, size_t size)
{
    assert(size != 0);

    bool was_empty = buf.empty();

    if (buf.capacity() < (buf.size() + size))
    {
        // we are going to copy anyway, so let us do a bit less of copying
        buf.erase(buf.begin(), buf.begin() + sent);
        sent = 0;
    }

    char const* cdata = static_cast<char const*>(data);
    buf.insert(buf.end(), cdata, cdata + size);

    if (was_empty)
        socket.set_on_write([this] { push_to_socket(); });
}

void send_buffer::push_to_socket()
{
    assert(!buf.empty());
    assert(sent < buf.size());

    size_t sent_now = socket.write_some(buf.data() + sent, buf.size() - sent);
    if (sent_now == 0)
        return;

    sent += sent_now;
    if (sent == buf.size())
    {
        buf.erase(buf.begin(), buf.end());
        sent = 0;
        socket.set_on_write(on_empty);
        if (on_empty)
            on_empty();
    }
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

http_server::outbound_connection::outbound_connection(http_server* parent, std::string const& host, client_socket::on_ready_t on_disconnect)
    : socket(try_connect(parent->ep, host, std::move(on_disconnect)))
    , send_buf(socket)
{}

void http_server::outbound_connection::send_request(http_request const& request)
{
    std::stringstream ss;
    ss << request;
    std::string str = ss.str();
    send_buf.append(str.data(), str.size());
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
    , send_buf(socket)
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
        target = std::make_unique<outbound_connection>(parent, host, [this] { drop(); });
        target->send_request(request);
        
    }

    std::string const& str = ss.str();
    send_buf.append(str.data(), str.size());
    send_buf.set_on_empty([this] { drop(); });
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
    send_buf.append(str.data(), str.size());
    send_buf.set_on_empty([this] { drop(); });
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
