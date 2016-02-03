#include <iostream>

#include "epoll.h"
#include "http_server.h"

int main()
{
    try
    {
        sysapi::epoll ep;
        http_server http_server(ep, ipv4_endpoint(0, ipv4_address::any()));

        ipv4_endpoint server_endpoint = http_server.local_endpoint();
        std::cout << "bound to " << server_endpoint << std::endl;

        ep.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "unknown exception in main" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

