#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "api.h"
#include "common/io.h"
#include "common/constants.h"
#define MSG_SIZE 84

int SESSION_ID, req_pipe, resp_pipe;

void read_wait(int file, char *buffer, size_t size){
  ssize_t ret = read(file, buffer, size);
  if (ret == -1) {
    fprintf(stderr, "Read failed\n");
    exit(EXIT_FAILURE);
  }
  while(buffer[0] == 0){
    ret = read(file, buffer, size);
    if (ret == -1) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
    }
  }
}

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  int fserv;
  char msg[84];
  char buffer[16] = {};
  fserv = open(server_pipe_path, O_WRONLY);
  if (fserv == -1) {
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }

  if ((unlink(req_pipe_path) != 0 && errno != ENOENT) || (unlink(resp_pipe_path) != 0 && errno != ENOENT)){
    fprintf(stderr, "unlink failed\n");
    exit(EXIT_FAILURE);
  }

  if ((mkfifo(req_pipe_path, 0777) < 0) || (mkfifo(resp_pipe_path, 0777) < 0)){
    fprintf(stderr, "mkfifo failed\n");
    exit(EXIT_FAILURE);
  } 
  if (req_pipe_path == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  if (resp_pipe_path == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  snprintf(msg, 84, "1 %s %s", req_pipe_path, resp_pipe_path);
  int chars_written = snprintf(msg, MSG_SIZE, "1 %s %s", req_pipe_path, resp_pipe_path);
  if (chars_written < 0 || chars_written >= MSG_SIZE) {
      fprintf(stderr, "Error formatting string\n");
      return 1;
  }
  memset(msg + chars_written, ' ', (size_t)(MSG_SIZE - chars_written - 1));
  send_msg(fserv, msg);

  req_pipe = open(req_pipe_path, O_WRONLY);
  resp_pipe = open(resp_pipe_path, O_RDONLY);
  if (req_pipe == -1 || resp_pipe == -1) {
    fprintf(stderr, "api-Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  // read_wait(resp_pipe, buffer, 16);
  read(resp_pipe,buffer,16);
  SESSION_ID = atoi(buffer);
  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  // send_msg(req_pipe, "2");
  int op = 2;
  if(write(req_pipe, &op, sizeof(int)) < 0){}
  /*FALTA DAR ERROS*/
  if (close(req_pipe) == -1 || close(resp_pipe) == -1) {
    fprintf(stderr,"Error closing pipe");
    exit(EXIT_FAILURE);
  }
  return 0;
}



int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // TODO: send create request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  // char buffer[16], msg[TAMMSG];
  // snprintf(msg, TAMMSG, "3 %u %zu %zu\n", event_id, num_rows, num_cols);
  // send_msg(req_pipe, msg);
  int return_value, op = 3;
  if(write(req_pipe, &op, sizeof(int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, &event_id, sizeof(unsigned int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, &num_rows, sizeof(size_t))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe,&num_cols, sizeof(size_t))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if (read(resp_pipe, &return_value, sizeof(int)) < 0) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
  }
  // read_wait(resp_pipe, buffer, 16);

  // int return_value = (atoi(buffer));
  return return_value;
}



int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // TODO: send reserve request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  int return_value, op = 4;
  if(write(req_pipe, &op, sizeof(int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, &event_id, sizeof(unsigned int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, &num_seats, sizeof(size_t))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, xs, sizeof(size_t) * MAX_RESERVATION_SIZE)<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, ys, sizeof(size_t) * MAX_RESERVATION_SIZE)<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if (read(resp_pipe, &return_value, sizeof(int)) < 0) {
    fprintf(stderr, "Read failed\n");
    exit(EXIT_FAILURE);
  }
  return return_value;
}



int ems_show(int out_fd, unsigned int event_id) {
  // TODO: send show request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  size_t num_columns, num_rows;
  unsigned int seats;
  char seat[16] = {};
  int return_value, op = 5;
  if(write(req_pipe, &op, sizeof(int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if(write(req_pipe, &event_id, sizeof(unsigned int))<0){
    fprintf(stderr, "write failed\n");
    return -1;////
  }
  if (read(resp_pipe, &return_value, sizeof(int)) < 0 || read(resp_pipe, &num_rows, sizeof(size_t)) < 0
    || read(resp_pipe, &num_columns, sizeof(size_t)) < 0) {
    fprintf(stderr, "Read failed\n");
    exit(EXIT_FAILURE);
  }
  if(return_value != 1){
    for (size_t i = 1; i <= num_rows; i++) {
      for (size_t j = 1; j <= num_columns; j++) {
        if (read(resp_pipe, &seats, sizeof(unsigned int)) < 0) {
          fprintf(stderr, "Read failed\n");
          exit(EXIT_FAILURE);
        }
        sprintf(seat, "%u" , seats);
        if (print_str(out_fd, seat)){
          perror("Error writing to file descriptor");
          return 1;
        }
        if (j < num_columns){
          if (print_str(out_fd," ")){
            perror("Error writing to file descriptor");
            return 1;
          }
        }
      }
      if (print_str(out_fd,"\n")){
        perror("Error writing to file descriptor");
        return 1;
      }
    }
  }
  return return_value;
}


int ems_list_events(int out_fd) {
  // TODO: send list request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char msg[20] = {};
  int return_value;
  size_t num_events;
  unsigned int ids;
  int op = 6;
  if(write(req_pipe, &op, sizeof(int))<0){
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  if (read(resp_pipe, &return_value, sizeof(int)) < 0 || read(resp_pipe, &num_events, sizeof(size_t)) < 0) {
    fprintf(stderr, "Read failed\n");
    exit(EXIT_FAILURE);
  }
  if(return_value != 1){
    if(num_events == 0){
      if (print_str(out_fd, "No events\n")){
        perror("Error writing to file descriptor");
        return 1;
      }
    }
    else{
      for (size_t i = 1; i <= num_events; i++) {
        if (read(resp_pipe, &ids, sizeof(unsigned int)) < 0) {
          fprintf(stderr, "Read failed\n");
          exit(EXIT_FAILURE);
        }
        sprintf(msg,"Event: %u\n",ids);
        if (print_str(out_fd, msg)){
          perror("Error writing to file descriptor");
          return 1;
        }
      }
    }
  }
  return return_value;
}
