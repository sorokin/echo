#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <map>
#include "socket.h"

struct echo_server
{
    struct connection
    {
        connection(echo_server* parent);

        void try_read();
        void try_write();

        void process(bool read);

    private:
        echo_server* parent;
        client_socket socket;
        timer_element timer;
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
    epoll& ep;
    server_socket ss;
    std::map<connection*, std::unique_ptr<connection>> connections;
};

#endif // ECHO_SERVER_H
