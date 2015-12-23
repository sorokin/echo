#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <map>
#include <memory>
#include "socket.h"


struct http_server
{
    struct browser_connection
    {
        browser_connection(http_server* parent);

        void new_request(char const* begin, char const* end);

    private:
        http_server* parent;
        client_socket socket;
        timer_element timer;
        size_t request_received;
        char request_buffer[4000];
        char responce_buffer[4000];
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
    std::map<browser_connection*, std::unique_ptr<browser_connection>> connections;
};

#endif // HTTP_SERVER_H
