#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <map>
#include <memory>
#include "socket.h"
#include "http_common.h"

struct send_buffer
{
    send_buffer(const send_buffer& other) = delete;
    send_buffer& operator=(const send_buffer& other) = delete;

    send_buffer(client_socket& socket);
    ~send_buffer();

    void append(void const* data, size_t size);
    void set_on_empty(client_socket::on_ready_t on_empty);

private:
    void append_to_buffer(void const* data, size_t size);
    void push_to_socket();

private:
    client_socket& socket;
    client_socket::on_ready_t on_empty;
    std::vector<char> buf;
    size_t sent;
};

struct http_server
{
    struct http_error : std::runtime_error
    {
        http_error(http_status_code status_code, std::string reason_phrase);

        http_status_code get_status_code() const;
        std::string const& get_reason_phrase() const;

    private:
        http_status_code status_code;
        std::string reason_phrase;
    };

    struct outbound_connection
    {
        outbound_connection(http_server* parent, std::string const& host, client_socket::on_ready_t on_disconnect);
        void send_request(http_request const& request);

    private:
        client_socket socket;
        send_buffer send_buf;
    };

    struct inbound_connection
    {
        inbound_connection(http_server* parent);

        void try_read();
        void drop();

    private:
        void new_request(char const* begin, char const* end);
        void send_header(http_status_code status_code, std::string const& message);

    private:
        http_server* parent;
        client_socket socket;
        timer_element timer;
        size_t request_received;
        char request_buffer[4000];
        send_buffer send_buf;

        std::unique_ptr<outbound_connection> target;
    };

    http_server(epoll& ep);
    http_server(epoll& ep, ipv4_endpoint const& local_endpoint);

    ipv4_endpoint local_endpoint() const;

private:
    void on_new_connection();

private:
    epoll& ep;
    server_socket ss;
    std::map<inbound_connection*, std::unique_ptr<inbound_connection>> connections;
};

#endif // HTTP_SERVER_H
