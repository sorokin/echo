#include "echo_tester.h"

#include <iostream>

echo_tester::connection::connection(echo_tester* parent, ipv4_endpoint const& remote, uint32_t number)
    : parent(parent)
    , socket(client_socket::connect(parent->ep, remote, [this] {
        std::cerr << "connection " << this->number << " was disconnected" << std::endl;
        this->parent->connections.erase(this);
    }))
    , timer([this] { goto_new_state(); })
    , number(number)
    , sent(0)
    , received(0)
{
    goto_new_state();
}

void echo_tester::connection::do_send()
{
    size_t sent_total = 0;

    uint8_t buf[2000];

    for (;;)
    {
        size_t to_send = rand() % 2000;
        fill_buffer(buf, to_send);
        size_t sent_now = socket.write_some(buf, to_send);
        sent += sent_now;
        sent_total += sent_now;
        if ((rand() % 10000) < 4000)
            break;
    }
}

void echo_tester::connection::do_receive()
{
    assert(sent >= received);

    uint8_t buf[2000];

    for (;;)
    {
        size_t bytes_received = socket.read_some(buf, sizeof buf);
        if (bytes_received == 0)
            break;
        if (!check_buffer(buf, bytes_received))
        {
            std::cout << "received data mismatch" << std::endl;
        }
    }
}

void echo_tester::connection::fill_buffer(uint8_t *buf, size_t size) const
{
    uint8_t val = (uint8_t)sent;
    for (size_t i = 0; i != size; ++i)
        *buf++ = val++;
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

void echo_tester::connection::goto_new_state()
{
    timer.restart(parent->ep.get_timer(), std::chrono::milliseconds(rand() % 450));
    state = (state_t)(rand() % (unsigned)state_t::max);
    //std::cout << "change connection " << number << " state to " << state_to_string(state) << std::endl;

    switch (state)
    {
    case state_t::idle:
        socket.set_on_read_write(client_socket::on_ready_t{}, client_socket::on_ready_t{});
        break;
    case state_t::sending_only:
        socket.set_on_read_write(client_socket::on_ready_t{}, [this] {
            do_send();
        });
        break;
    case state_t::receiving_only:
        socket.set_on_read_write([this] {
            do_receive();
        }, client_socket::on_ready_t{});
        break;
    case state_t::sending_receiving:
        socket.set_on_read_write([this] {
            do_receive();
        }, [this] {
            do_send();
        });
        break;
    default:
        assert(false);
        break;
    }
}

const char* echo_tester::connection::state_to_string(state_t state)
{
    switch (state)
    {
    case state_t::idle:
        return "idle";
    case state_t::sending_only:
        return "sending_only";
    case state_t::receiving_only:
        return "receiving_only";
    case state_t::sending_receiving:
        return "sending_receiving";
    default:
        assert(false);
        return "<invalid state>";
    }
}

uint32_t echo_tester::connection::get_number() const
{
    return number;
}

echo_tester::echo_tester(epoll &ep, ipv4_endpoint remote_endpoint)
    : ep(ep)
    , timer([this] { tick(); })
    , remote_endpoint(remote_endpoint)
    , next_connection_number(0)
    , desired_number_of_connections(500)
    , number_of_permanent_connections(0)
{
    tick();
}

void echo_tester::tick()
{
    size_t i = rand() % (desired_number_of_connections * 2);
    if (i < connections.size())
    {
        auto it = std::next(connections.begin(), i);
        std::cerr << "kill connection " << it->second->get_number() << std::endl;
        connections.erase(it);
    }
    else
    {
        uint32_t id = next_connection_number;
        try
        {
            std::unique_ptr<connection> cc(new connection(this, this->remote_endpoint, id));
            connection* pcc = cc.get();
            connections.emplace(pcc, std::move(cc));
            next_connection_number++;
            std::cerr << "new connection " << id << std::endl;
        }
        catch (std::exception const& e)
        {
            std::cerr << "failed to create new connection: " << e.what() << std::endl;
        }
    }
    timer.restart(this->ep.get_timer(), std::chrono::milliseconds(rand() % 40));
}
