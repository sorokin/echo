#include "pipe.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "throw_error.h"

pipe_pair make_pipe(bool non_block)
{
    int fds[2];
    int res = pipe2(fds, O_CLOEXEC | (non_block ? O_NONBLOCK : 0));

    if (res != 0)
    {
        assert(res == -1);
        throw_error(errno, "pipe2()");
    }

    return pipe_pair{weak_file_descriptor{fds[0]}, weak_file_descriptor{fds[1]}};
}
