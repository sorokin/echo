#include <iostream>
#include "file_descriptor.h"

#include "socket.h"
#include <sys/socket.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/mman.h>

#include "epoll.h"

#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <thread>

struct echo_server
{
    struct connection
    {
        connection(echo_server* parent)
            : parent(parent)
            , socket(parent->ss.accept([this] {
                this->parent->connections.erase(this);
            }))
            , start_offset()
            , end_offset()
        {
            update();
        }

        void update()
        {
                assert(start_offset == 0);
                socket.set_on_read([this] {
                    end_offset = socket.read_some(buf, sizeof buf);
                    assert(start_offset == 0);
                    start_offset += socket.write_some(buf + start_offset, end_offset - start_offset);
                    if (start_offset == end_offset)
                    {
                        start_offset = 0;
                        end_offset = 0;
                    }
                });
        }

    private:
        echo_server* parent;
        client_socket socket;
        size_t start_offset;
        size_t end_offset;
        char buf[1500];
    };

    echo_server(epoll& ep)
        : ss{ep, std::bind(&echo_server::on_new_connection, this)}
    {}

    echo_server(epoll& ep, ipv4_endpoint const& local_endpoint)
        : ss{ep, local_endpoint, std::bind(&echo_server::on_new_connection, this)}
    {}

    ipv4_endpoint local_endpoint() const
    {
        return ss.local_endpoint();
    }

private:
    void on_new_connection()
    {
        std::unique_ptr<connection> cc(new connection(this));
        connection* pcc = cc.get();
        connections.emplace(pcc, std::move(cc));
    }

private:
    server_socket ss;
    std::map<connection*, std::unique_ptr<connection>> connections;
};


int main()
{
    sysapi::epoll ep;
    echo_server echo_server(ep, ipv4_endpoint(0, ipv4_address::any()));

    ipv4_endpoint echo_server_endpoint = echo_server.local_endpoint();
    std::cout << "bound to " << echo_server_endpoint << std::endl;

    /*std::thread t{[echo_server_endpoint]{
        sysapi::epoll tep;
        echo_tester tester{tep, echo_server_endpoint};
        tester.do_n_steps(100000);
        std::cerr << "finished" << std::endl;
    }};*/
    ep.run();

    //t.join();
    return 0;
}
