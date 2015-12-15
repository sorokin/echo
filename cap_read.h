#ifndef CAP_READ_H
#define CAP_READ_H

#include "file_descriptor.h"

#include <array>
#include <string>

size_t read_some(weak_file_descriptor fd, void* data, size_t size);

#endif // CAP_READ_H
