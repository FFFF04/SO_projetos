#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <pthread.h>
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

typedef struct {
  int event_id;
  pthread_mutex_t write_file_lock;
}locks;

void del(int n, unsigned int *arr);
int find_event_id(unsigned int event_id);
void lock_comando();
void unlock_comando();
void lock_active();
void unlock_active();
void read_lock();
void read_unlock();
void write_lock();
void write_unlock();
void writing_lock(unsigned int event_id);
void writing_unlock(unsigned int event_id);
void read_files(char* path, char* name , int max_threads);
void* process(void* arg);
void barrier_wait();
#endif  // PROCESS_H
