#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include "constants.h"
typedef struct{
  int file;
  int file_out;
  int command;
  unsigned int event_id;
  unsigned int delay;
  size_t num_rows;
  size_t num_columns;
  size_t num_coords;
  size_t xs[MAX_RESERVATION_SIZE];
  size_t ys[MAX_RESERVATION_SIZE];

}data;
void* process(void* arg);
#endif  // PROCESS_H
