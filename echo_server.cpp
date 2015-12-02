#include "echo_server.h"

echo_server::connection::connection(echo_server *parent)
    : parent(parent)
    , socket(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }, [this] {
        process(true);
    }, client_socket::on_ready_t{}))
    , start_offset()
    , end_offset()
{}

void echo_server::connection::try_read()
{
    assert(start_offset == 0);
    assert(end_offset == 0);
    end_offset = socket.read_some(buf, sizeof buf);
}

void echo_server::connection::try_write()
{
    assert(start_offset < end_offset);
    assert(end_offset != 0);

    start_offset += socket.write_some(buf + start_offset, end_offset - start_offset);
    if (start_offset == end_offset)
    {
        start_offset = 0;
        end_offset = 0;
    }
}

void echo_server::connection::process(bool read)
{
    bool incomplete_read = false;

    if (!read)
        goto process_write;

    for (;;)
    {
        try_read();
        if (end_offset == 0)
        {
            socket.set_on_read_write([this] { process(true); }, client_socket::on_ready_t{});
            return;
        }

        incomplete_read = (end_offset != sizeof buf);

    process_write:
        try_write();
        if (end_offset != 0)
        {
            socket.set_on_read_write(client_socket::on_ready_t{}, [this] { process(false); });
            return;
        }

        if (incomplete_read)
        {
            socket.set_on_read_write([this] { process(true); }, client_socket::on_ready_t{});
            return;
        }
    }
}

echo_server::echo_server(epoll &ep)
    : ss{ep, std::bind(&echo_server::on_new_connection, this)}
{}

echo_server::echo_server(epoll &ep, ipv4_endpoint const& local_endpoint)
    : ss{ep, local_endpoint, std::bind(&echo_server::on_new_connection, this)}
{}

ipv4_endpoint echo_server::local_endpoint() const
{
    return ss.local_endpoint();
}

void echo_server::on_new_connection()
{
    std::unique_ptr<connection> cc(new connection(this));
    connection* pcc = cc.get();
    connections.emplace(pcc, std::move(cc));
}
