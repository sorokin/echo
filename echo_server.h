#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <map>
#ifdef __APPLE__
#include "socket_apple.h"
#else
#include "socket.h"
#endif

struct echo_server
{
    struct connection
    {
        connection(echo_server* parent);
        void update();

    private:
        echo_server* parent;
        client_socket socket;
        size_t start_offset;
        size_t end_offset;
        char buf[1500];
    };

    echo_server(epoll& ep);

    echo_server(epoll& ep, ipv4_endpoint const& local_endpoint);

    ipv4_endpoint local_endpoint() const;

private:
    void on_new_connection();

private:
    server_socket ss;
    std::map<connection*, std::unique_ptr<connection>> connections;
};

#endif // ECHO_SERVER_H
