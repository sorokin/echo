#include "pipe.h"

#include <unistd.h>
#include <fcntl.h>

#include "throw_error.h"

pipe_pair make_pipe(bool non_block)
{
    int fds[2];
    
    int res = pipe(fds);

    if (res != 0)
    {
        assert(res == -1);
        throw_error(0, "pipe()"); // undeclarated errno?
    }

    return pipe_pair{weak_file_descriptor{fds[0]}, weak_file_descriptor{fds[1]}};
}
