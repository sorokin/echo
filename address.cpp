#include "address.h"

#include <arpa/inet.h>
#include <netinet/in.h>

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


std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint)
{
    in_addr tmp{};
    tmp.s_addr = endpoint.addr_net;
    os << inet_ntoa(tmp) << ':' << endpoint.port();
    return os;
}
