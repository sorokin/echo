#ifndef THROW_ERROR_H
#define THROW_ERROR_H

void throw_error [[noreturn]] (int err, char const* action);

#endif // THROW_ERROR_H
