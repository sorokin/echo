#ifndef ADDRESS_H
#define ADDRESS_H

#include <sys/socket.h>
#include <cstdint>
#include <string>
#include <ostream>
#include <vector>

struct ipv4_address
{
    ipv4_address();
    ipv4_address(uint32_t addr_net);
    ipv4_address(std::string const& text);

    std::string to_string() const;

    uint32_t address_network() const;

    static ipv4_address any();
    static std::vector<ipv4_address> resolve(std::string const& hostname);

private:
    uint32_t addr_net;

    friend std::ostream& operator<<(std::ostream& os, ipv4_address const& addr);
    friend struct ipv4_endpoint;
};

struct ipv4_endpoint
{
    ipv4_endpoint();
    ipv4_endpoint(uint16_t port_host, ipv4_address);

    uint16_t port() const;
    ipv4_address address() const;

    std::string to_string() const;

private:
    ipv4_endpoint(uint16_t port_net, uint32_t addr_net);

    uint16_t port_net;
    uint32_t addr_net;

    friend std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint);
    friend struct client_socket;
    friend struct server_socket;
};

std::ostream& operator<<(std::ostream& os, ipv4_address const& addr);
std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint);

#endif // ADDRESS_H
