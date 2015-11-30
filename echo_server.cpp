#include "echo_server.h"

echo_server::connection::connection(echo_server *parent)
    : parent(parent)
    , socket(parent->ss.accept([this] {
        this->parent->connections.erase(this);
    }))
    , start_offset()
    , end_offset()
{
    update();
}

void echo_server::connection::update()
{
    if (end_offset == 0)
    {
        assert(start_offset == 0);
        socket.set_on_read([this] {
            end_offset = socket.read_some(buf, sizeof buf);
            assert(start_offset == 0);
            update();
        });
        socket.set_on_write([this]{});  // otherwise exception will be throwen
    }
    else
    {
        assert(start_offset < end_offset);
        socket.set_on_read([this]{});
        socket.set_on_write([this] {
            start_offset += socket.write_some(buf + start_offset, end_offset - start_offset);
            if (start_offset == end_offset)
            {
                start_offset = 0;
                end_offset = 0;
                update();
            }
        });
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
