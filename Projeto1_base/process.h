#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include "constants.h"

typedef struct{
  int file;
  int file_out;
}data;

void read_files(char* path, char* name , int max_threads);
void* process(void* arg);
#endif  // PROCESS_H
