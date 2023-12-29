#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <pthread.h>
#include <stddef.h>

// void initialize_lock();
// void lock_lock();
// void lock_unlock();
void initialize();
void lock(int file_descriptor);
void unlock(int file_descriptor);
int get_to_show();
void set_to_show();
/// Parses an unsigned integer from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param value Pointer to the variable to store the value in.
/// @param next Pointer to the variable to store the next character in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_uint(int fd, unsigned int *value, char *next);

/// Prints an unsigned integer to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_uint(int fd, unsigned int value);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str(int fd, const char *str);

#endif  // COMMON_IO_H
