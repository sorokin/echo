#include "address.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cassert>
#include <sstream>
#include <stdexcept>

ipv4_address::ipv4_address(uint32_t addr_net)
    : addr_net(addr_net)
{}

ipv4_address::ipv4_address(const std::string &text)
{
    in_addr tmp{};
    int res = inet_pton(AF_INET, text.c_str(), &tmp);
    if (res == 0)
    {
        std::stringstream ss;
        ss << '\'' << text << "' is not a valid ip address";
        throw std::runtime_error(ss.str());
    }
    addr_net = tmp.s_addr;
}

std::string ipv4_address::to_string() const
{
    in_addr tmp{};
    tmp.s_addr = addr_net;
    return inet_ntoa(tmp);
}

uint32_t ipv4_address::address_network() const
{
    return addr_net;
}

ipv4_address ipv4_address::any()
{
    return ipv4_address{INADDR_ANY};
}

std::vector<ipv4_address> ipv4_address::resolve(std::string const& hostname)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* list_head;
    int res = getaddrinfo(hostname.c_str(), nullptr, &hints, &list_head);
    if (res != 0)
    {
        std::stringstream ss;
        ss << "can not resolve server '" << hostname << "': " << gai_strerror(res);
        throw std::runtime_error(ss.str());
    }

    std::vector<ipv4_address> r;

    for (addrinfo* i = list_head; i != nullptr; i = i->ai_next)
    {
        assert(i->ai_family == AF_INET);
        assert(i->ai_socktype == SOCK_STREAM);
        r.push_back(ipv4_address{reinterpret_cast<sockaddr_in const*>(i->ai_addr)->sin_addr.s_addr});
    }

    // TODO: move to destructor
    freeaddrinfo(list_head);

    return r;
}

ipv4_endpoint::ipv4_endpoint()
    : port_net{}
    , addr_net{}
{}

ipv4_endpoint::ipv4_endpoint(uint16_t port_host, ipv4_address addr)
    : port_net{htons(port_host)}
    , addr_net{addr.addr_net}
{}

uint16_t ipv4_endpoint::port() const
{
    return ntohs(port_net);
}

ipv4_address ipv4_endpoint::address() const
{
    return ipv4_address{addr_net};
}

std::string ipv4_endpoint::to_string() const
{
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

ipv4_endpoint::ipv4_endpoint(uint16_t port_net, uint32_t addr_net)
    : port_net(port_net)
    , addr_net(addr_net)
{}

std::ostream& operator<<(std::ostream& os, ipv4_address const& addr)
{
    in_addr tmp{};
    tmp.s_addr = addr.addr_net;
    os << inet_ntoa(tmp);
    return os;
}

std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint)
{
    in_addr tmp{};
    tmp.s_addr = endpoint.addr_net;
    os << inet_ntoa(tmp) << ':' << endpoint.port();
    return os;
}
