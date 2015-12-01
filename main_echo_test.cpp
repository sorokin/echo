#include <iostream>

#ifdef __APPLE__
#include "kqueue.hpp"
#else
#include "epoll.h"
#endif
#include "echo_tester.h"

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " hostname port\n";
        return 1;
    }

    try
    {
        ipv4_endpoint endpoint(std::stod(argv[2]), ipv4_address(argv[1]));
        std::cout << endpoint << std::endl;
        sysapi::epoll tep;
        echo_tester tester{tep, endpoint};
        tester.do_n_steps(1000000);
    }
    catch (std::exception const& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "unknown exception in main" << std::endl;
    }

    return 0;
}
