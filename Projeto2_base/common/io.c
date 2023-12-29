#include "io.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
// #include <pthread.h>

// pthread_mutex_t fifo_lock;
int to_show = 0;

// void initialize_lock(){
//   if (pthread_mutex_init(&fifo_lock,NULL) != 0){
//     fprintf(stderr, "Error in pthread_mutex_init()\n");
//     exit(EXIT_FAILURE);
//   }
// }

// void lock_lock(){
//   if (pthread_mutex_lock(&fifo_lock) != 0){
//     fprintf(stderr, "Error in pthread_mutex_lock()\n");
//     exit(EXIT_FAILURE);
//   }
// }

// void lock_unlock(){
//   if (pthread_mutex_unlock(&fifo_lock) != 0){
//     fprintf(stderr, "Error in pthread_mutex_unlock()\n");
//     exit(EXIT_FAILURE);
//   }
// }

struct flock fl;

void initialize(){
  fl.l_type = F_WRLCK;  // Exclusive write lock
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;  // Lock the entire file
}

void lock(int file_descriptor){
  fcntl(file_descriptor, F_SETLKW, &fl);
}

void unlock(int file_descriptor){
  fl.l_type = F_UNLCK;
  fcntl(file_descriptor, F_SETLK, &fl);
}

int get_to_show(){
  return to_show;
}

void set_to_show(){
  to_show = 1;
}

int parse_uint(int fd, unsigned int *value, char *next) {
  char buf[16];

  int i = 0;
  while (1) {
    ssize_t read_bytes = read(fd, buf + i, 1);
    if (read_bytes == -1) {
      return 1;
    } else if (read_bytes == 0) {
      *next = '\0';
      break;
    }

    *next = buf[i];

    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }

    i++;
  }

  unsigned long ul = strtoul(buf, NULL, 10);

  if (ul > UINT_MAX) {
    return 1;
  }

  *value = (unsigned int)ul;

  return 0;
}

int print_uint(int fd, unsigned int value) {
  char buffer[16];
  size_t i = 16;

  for (; value > 0; value /= 10) {
    buffer[--i] = '0' + (char)(value % 10);
  }

  if (i == 16) {
    buffer[--i] = '0';
  }

  while (i < 16) {
    ssize_t written = write(fd, buffer + i, 16 - i);
    if (written == -1) {
      return 1;
    }

    i += (size_t)written;
  }

  return 0;
}

int print_str(int fd, const char *str) {
  size_t len = strlen(str);
  while (len > 0) {
    ssize_t written = write(fd, str, len);
    if (written == -1) {
      return 1;
    }

    str += (size_t)written;
    len -= (size_t)written;
  }

  return 0;
}
