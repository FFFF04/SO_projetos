#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include "parser.h"
#include "constants.h"

typedef struct{
  int file;
  int file_out;
  int thread_id;
}data;

typedef struct {
  int thread_id;
  int size;
  unsigned int *waiting_time;
}waiting_list;

void read_files(char* path, char* name , int max_threads);
void* process(void* arg);
void barrier_wait();
#endif  // PROCESS_H
