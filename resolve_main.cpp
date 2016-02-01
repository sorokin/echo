#include "address.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <hostname>" << std::endl;
        return EXIT_SUCCESS;
    }

    try
    {
        for (ipv4_address const& addr : ipv4_address::resolve(argv[1]))
        {
            std::cout << addr << std::endl;
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
