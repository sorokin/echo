#ifndef ECHO_TESTER_H
#define ECHO_TESTER_H

#include "epoll.h"
#include "address.h"
#include "socket.h"
#include <vector>

struct echo_tester
{
    struct connection
    {
        connection(epoll& ep, ipv4_endpoint const& remote, uint32_t number);

        void do_send();
        bool do_receive();

    private:
        void fill_buffer(uint8_t* buf, size_t size);
        bool check_buffer(uint8_t* buf, size_t size);

    public:
        uint32_t get_number() const;

    private:
        client_socket socket;
        uint32_t number;
        uint64_t sent;
        uint64_t received;
    public:
        bool no_disconnect;
        bool no_read;
    };

    echo_tester(epoll& ep, ipv4_endpoint remote_endpoint);
    bool do_step();
    void do_n_steps(size_t n);

private:
    epoll& ep;
    ipv4_endpoint remote_endpoint;
    std::vector<std::unique_ptr<connection>> connections;
    uint32_t next_connection_number;
    size_t desired_number_of_connections;
};

#endif // ECHO_TESTER_H
