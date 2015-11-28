#include "echo_tester.h"

#include <iostream>

echo_tester::connection::connection(epoll &ep, ipv4_endpoint const& remote, uint32_t number)
    : socket(client_socket::connect(ep, remote, [] {}))
    , number(number)
    , sent(0)
    , received(0)
{}

void echo_tester::connection::do_send()
{
    size_t size = rand() % 10000;

    std::cout << "sending " << size << " bytes over connection " << number << std::endl;
    uint8_t buf[2000];

    while (size != 0)
    {
        size_t to_send = std::min(size, sizeof buf);
        fill_buffer(buf, to_send);
        socket.write_some(buf, to_send);
        size -= to_send;
    }
}

bool echo_tester::connection::do_receive()
{
    assert(sent >= received);
    if (sent == received)
        return true;

    //std::cout << "receiving " << (sent - received) << " bytes over connection " << number << std::endl;

    size_t size = rand() % (sent - received);

    uint8_t buf[2000];

    while (size != 0)
    {
        size_t to_receive = std::min(size, sizeof buf);
        size_t bytes_received = socket.read_some(buf, to_receive);
        if (bytes_received == 0)
        {
            std::cerr << "incomplete read (connection " << number << "), requested: " << to_receive << ", received: " << bytes_received << std::endl;
            return false;
        }
        if (!check_buffer(buf, bytes_received))
            return false;
        size -= bytes_received;
    }

    return true;
}

void echo_tester::connection::fill_buffer(uint8_t *buf, size_t size)
{
    for (size_t i = 0; i != size; ++i)
        *buf++ = (uint8_t)sent++;
}

bool echo_tester::connection::check_buffer(uint8_t *buf, size_t size)
{
    for (size_t i = 0; i != size; ++i)
    {
        if (*buf != (uint8_t)received)
        {
            std::cerr << "mismatch, expected: " << (uint32_t)(uint8_t)received << "(" << received << "), received: " << (uint32_t)*buf << std::endl;
            return false;
        }
        ++buf;
        ++received;
    }

    return true;
}

uint32_t echo_tester::connection::get_number() const
{
    return number;
}

echo_tester::echo_tester(epoll &ep, ipv4_endpoint remote_endpoint)
    : ep(ep)
    , remote_endpoint(remote_endpoint)
    , next_connection_number(0)
{}

bool echo_tester::do_step()
{
    size_t i = rand() % 5;
    if (i < connections.size())
    {
        size_t act = rand() % 11;
        if (act == 10)
        {
            //std::cout << "destroy connection " << connections[i]->get_number() << std::endl;
            std::swap(connections[i], connections.back());
            connections.pop_back();
        }
        else if (act < 5)
        {
            auto& c = *connections[i];
            c.do_send();
        }
        else
        {
            auto& c = *connections[i];
            if (!c.do_receive())
                return false;
        }
    }
    else
    {
        std::unique_ptr<connection> cc(new connection(ep, remote_endpoint, next_connection_number++));
        connections.push_back(std::move(cc));
    }
    return true;
}

void echo_tester::do_n_steps(size_t n)
{
    for (size_t i = 0; i != n; ++i)
        if (!do_step())
            break;
}
