#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <map>
#include <experimental/optional>
#include "socket.h"

struct http_server
{
    struct connection
    {
        connection(http_server* parent);

    private:
        http_server* parent;
        client_socket browser;
        std::experimental::optional<client_socket> target_server;
        timer_element timer;
    };

    http_server(epoll& ep);
    http_server(epoll& ep, ipv4_endpoint const& local_endpoint);

    ipv4_endpoint local_endpoint() const;

private:
    void on_new_connection();

private:
    epoll& ep;
    server_socket ss;
    std::map<connection*, std::unique_ptr<connection>> connections;
};

#endif // HTTP_SERVER_H

