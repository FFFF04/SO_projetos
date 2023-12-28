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

int SESSION_ID, req_pipe, resp_pipe;
char *req_pipe_nome;
char *resp_pipe_nome;

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
  char msg[TAMMSG];
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

  req_pipe_nome = (char*) malloc(strlen(req_pipe_path) + 1);
  resp_pipe_nome = (char*) malloc(strlen(resp_pipe_path) + 1);
  strncpy(req_pipe_nome, req_pipe_path, strlen(req_pipe_path) + 1);
  strncpy(resp_pipe_nome, resp_pipe_path, strlen(resp_pipe_path) + 1);
  ssize_t ret;
  snprintf(msg, TAMMSG, "1 %s %s \n", req_pipe_path, resp_pipe_path);

  pthread_mutex_t fifo_lock  = getlock();
  if (pthread_mutex_lock(&fifo_lock) != 0) {
    fprintf(stderr, "Error locking\n");
    return 1;
  }
  ret = write(fserv, msg, strlen(msg) + 1);
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  // unlock_fifo()
  // pthread_rwlock_unlock(&fifo_lock);

  req_pipe = open(req_pipe_path, O_WRONLY);
  resp_pipe = open(resp_pipe_path, O_RDONLY);
  if (req_pipe == -1 || resp_pipe == -1) {
    fprintf(stderr, "api-Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  read_wait(resp_pipe, buffer, 16);
  SESSION_ID = atoi(buffer);
  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  if(!get_to_show()){
    char msg[TAMMSG];
    snprintf(msg, TAMMSG, "2");
    ssize_t ret = write(req_pipe, msg, strlen(msg) + 1);
    if (ret < 0) {
      fprintf(stderr, "Write failed\n");
      exit(EXIT_FAILURE);
    }
  }
  /*FALTA DAR ERROS*/
  free(req_pipe_nome);
  free(resp_pipe_nome);
  close(req_pipe);
  unlink(req_pipe_nome);
  close(resp_pipe);
  unlink(resp_pipe_nome);
  return 0;
}



int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // TODO: send create request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[16], msg[TAMMSG];
  ssize_t ret;
  snprintf(msg, TAMMSG, "3 %u %zu %zu\n", event_id, num_rows, num_cols);
  ret = write(req_pipe, msg, strlen(msg) + 1);
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  read_wait(resp_pipe, buffer, 16);

  int return_value = (atoi(buffer));
  return return_value;
}



int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // TODO: send reserve request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG], seat[16] = {};
  ssize_t ret;
  snprintf(msg, TAMMSG, "4 %u %zu ", event_id, num_seats);
  for (size_t i = 0; i < num_seats; i++){
    snprintf(seat, 16, "%ld %ld ", xs[i], ys[i]);
    strcat(msg,seat);
    memset(seat,0,16);
  }
  
  ret = write(req_pipe, msg, strlen(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  read_wait(resp_pipe, buffer, TAMMSG);

  int return_value = (atoi(strtok(buffer, " ")));

  return return_value;
}



int ems_show(int out_fd, unsigned int event_id) {
  // TODO: send show request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  snprintf(msg, TAMMSG, "5 %u ", event_id);
  ssize_t ret;
  ret = write(req_pipe, msg, strlen(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  read_wait(resp_pipe, buffer, TAMMSG);
  ret = (size_t)(atoi(strtok(buffer, " ")));
  if(ret != 1){
    size_t num_rows = (size_t)(atoi(strtok(NULL, " ")));
    size_t num_columns = (size_t)(atoi(strtok(NULL, " ")));
    char *mensagem = strtok(NULL,"|");
    ret = write(out_fd, mensagem, (num_columns*num_rows*2));
    if (ret == -1){
      fprintf(stderr, "write failed\n");
      exit(EXIT_FAILURE);
    }
    return 0;
  }
  return 1;
}


/*FALTA DAR ERROS*/
int ems_list_events(int out_fd) {
  // TODO: send list request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  snprintf(msg, TAMMSG, "6");
  ssize_t ret;
  ret = write(req_pipe, msg, strlen(msg) + 1);
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  read_wait(resp_pipe, buffer, TAMMSG);
  ret = (atoi(strtok(buffer, " ")));
  if(ret != 1){
    size_t num_events = (size_t)(atoi(strtok(NULL, " ")));
    //char *mensagem = (char*) malloc((num_events)+1);
    while(num_events != 0){
      char *mensagem = strtok(NULL,"|");
      ret = write(out_fd, mensagem, strlen(mensagem));
      if (ret == -1){
        fprintf(stderr, "write failed\n");
        exit(EXIT_FAILURE);
      }
      num_events--;
      memset(mensagem,0,strlen(mensagem));
    }
    //free(mensagem); 
    return 0;
    //unsigned int *ids = strtok(buffer, " ");///memoria
  }

  return 1;
}
