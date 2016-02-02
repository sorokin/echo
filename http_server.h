#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <map>
#include <memory>
#include "socket.h"
#include "http_common.h"

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

    struct inbound_connection
    {
        inbound_connection(http_server* parent);

        void try_read();
        void drop();

    private:
        void new_request(char const* begin, char const* end);
        void try_write();
        void send_header(http_status_code status_code, std::string const& message);

    private:
        http_server* parent;
        client_socket socket;
        timer_element timer;
        size_t request_received;
        char request_buffer[4000];
        size_t response_size;
        size_t response_sent;
        char response_buffer[4000];

        std::unique_ptr<client_socket> target;
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
