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
#include "echo_tester.h"

#include <map>
#include <memory>
#include <functional>
#include <vector>


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " <ip-address> <port>\n";
        return 1;
    }


    ipv4_endpoint endpoint(std::stod(argv[2]), ipv4_address(argv[1]));
    std::cout << endpoint << std::endl;
    sysapi::epoll tep;
    echo_tester tester{tep, endpoint};
    tester.do_n_steps(100000);

    return 0;
}
