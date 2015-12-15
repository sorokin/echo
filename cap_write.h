#ifndef CAP_WRITE_H
#define CAP_WRITE_H

#include "file_descriptor.h"

size_t write_some(weak_file_descriptor fd, void const* data, std::size_t size);
void write(weak_file_descriptor fdc, const void *data, std::size_t size);

#endif // CAP_WRITE_H
