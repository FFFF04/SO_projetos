#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include "parser.h"
#include "constants.h"

typedef struct{
  int file;
  int file_out;
  int command;
  int max_threads;
}data;

/*typedef struct waiting_list{
  int thread_id;
  int waiting_time;
  struct waiting_list* next;
}waiting_list;*/

void read_files(char* path, char* name , int max_threads);
void* process(void* arg);
void barrier_wait();
#endif  // PROCESS_H
