#ifndef ECHO_TESTER_H
#define ECHO_TESTER_H

#include "epoll.h"
#include "address.h"
#include "socket.h"
#include <map>

struct echo_tester
{
    struct connection
    {
        enum class state_t
        {
            idle,
            sending_only,
            receiving_only,
            sending_receiving,

            max
        };

        connection(echo_tester* parent, ipv4_endpoint const& remote, uint32_t number);

        void do_send();
        void do_receive();

    private:
        void fill_buffer(uint8_t* buf, size_t size) const;
        bool check_buffer(uint8_t* buf, size_t size);
        void goto_new_state();
        static char const* state_to_string(state_t);

    public:
        uint32_t get_number() const;

    private:
        echo_tester* parent;
        client_socket socket;
        timer_element timer;
        uint32_t number;
        uint64_t sent;
        uint64_t received;
        state_t state;
    };

    echo_tester(epoll& ep, ipv4_endpoint remote_endpoint);

private:
    void tick();

private:
    epoll& ep;
    timer_element timer;
    ipv4_endpoint remote_endpoint;
    std::map<connection*, std::unique_ptr<connection>> connections;
    uint32_t next_connection_number;
    size_t desired_number_of_connections;
    size_t number_of_permanent_connections;
};

#endif // ECHO_TESTER_H
