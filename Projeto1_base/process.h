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

void read_files(char* path, char* name , int max_threads);
void* process(void* arg);
void barrier_wait();
#endif  // PROCESS_H
