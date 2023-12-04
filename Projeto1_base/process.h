#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
typedef struct{
  int file;
  int file_out;
  int command;
  unsigned int event_id;
  size_t num_rows;
  size_t num_columns;
  size_t num_coords;
}data;
void process(data valores);
#endif  // PROCESS_H
