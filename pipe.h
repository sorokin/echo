#ifndef PIPE_H
#define PIPE_H

#include "file_descriptor.h"

struct pipe_pair
{
    weak_file_descriptor out;
    weak_file_descriptor in;
};

pipe_pair make_pipe(bool non_block);

#endif // PIPE_H
