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

#define TAMMSG 1000
int SESSION_ID, req_pipe, resp_pipe;
char *req_pipe_nome;
char *resp_pipe_nome;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  int fserv;
  char buffer[TAMMSG], msg[TAMMSG];
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

  req_pipe = open(req_pipe_path, O_WRONLY);
  resp_pipe = open(resp_pipe_path, O_RDONLY);
  if (req_pipe == -1 || resp_pipe == -1) {
    fprintf(stderr, "Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  strcpy(req_pipe_nome,req_pipe_path);
  strcpy(resp_pipe_nome,resp_pipe_path);
  ssize_t ret;
  snprintf(msg, TAMMSG, "1 %s %s ", req_pipe_path, resp_pipe_path);
  ret = write(fserv, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  sleep(1);//ACHO QUE É NECESSAIRO EM TUDO PORQUE NAO PODEMOS ESTAR LER LOGO ENQUANTO O SERVIDOR AINDA ESTA A TENTAR METER NO PIPE
  ret = read(fserv, buffer, TAMMSG - 1);
  if (ret == 0) {
    fprintf(stderr, "Pipe closed\n");
    exit(EXIT_SUCCESS);
  } else if (ret == -1) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
  }
  SESSION_ID = atoi(buffer);
  return 1;
}



int ems_quit(void) { 
  //TODO: close pipes
  char msg[TAMMSG];
  snprintf(msg, TAMMSG, "2");
  ssize_t ret = write(req_pipe, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  close(req_pipe);
  unlink(req_pipe_nome);
  close(resp_pipe);
  unlink(resp_pipe_nome);
  return 1;
}



int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  // TODO: send create request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  ssize_t ret;
  snprintf(msg, TAMMSG, "3 %u %zu %zu ", event_id, num_rows, num_cols);
  ret = write(req_pipe, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }
  
  ret = read(resp_pipe, buffer, TAMMSG - 1);
  if (ret == 0) {
    fprintf(stderr, "pipe closed\n");
    exit(EXIT_FAILURE);
  } else if (ret == -1) {
    fprintf(stderr, "read failed\n");
    exit(EXIT_FAILURE);
  }
  ret = (atoi(strtok(buffer, " ")));
  return 1;
}



int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  // TODO: send reserve request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  ssize_t ret;
  snprintf(msg, TAMMSG, "4 %u %zu %ln %ln ", event_id, num_seats, xs, ys);
  ret = write(req_pipe, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  ret = read(resp_pipe, buffer, TAMMSG - 1);
  if (ret == 0) {
    fprintf(stderr, "pipe closed\n");
    exit(EXIT_FAILURE);
  } else if (ret == -1) {
    fprintf(stderr, "read failed\n");
    exit(EXIT_FAILURE);
  }
  ret = (atoi(strtok(buffer, " ")));

  return 1;
}



int ems_show(int out_fd, unsigned int event_id) {
  // TODO: send show request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  snprintf(msg, TAMMSG, "5 %u ", event_id);
  ssize_t ret;
  ret = write(req_pipe, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  ret = read(resp_pipe, buffer, TAMMSG - 1);
  if (ret == 0) {
    fprintf(stderr, "pipe closed\n");
    exit(EXIT_FAILURE);
  } else if (ret == -1) {
    fprintf(stderr, "read failed\n");
    exit(EXIT_FAILURE);
  }
  ret = (atoi(strtok(buffer, " ")));
  if(ret != 1){
    size_t num_rows = (size_t)(atoi(strtok(buffer, " ")));
    size_t num_columns = (size_t)(atoi(strtok(buffer, " ")));
    size_t* xs = (size_t*)(strtok(buffer, " "));///memoria
  }

  return 1;
}



int ems_list_events(int out_fd) {
  // TODO: send list request to the server (through the request pipe) 
  // and wait for the response (through the response pipe)
  char buffer[TAMMSG], msg[TAMMSG];
  snprintf(msg, TAMMSG, "6");
  ssize_t ret;
  ret = write(req_pipe, msg, sizeof(msg));
  if (ret < 0) {
    fprintf(stderr, "Write failed\n");
    exit(EXIT_FAILURE);
  }

  ret = read(resp_pipe, buffer, TAMMSG - 1);
  if (ret == 0) {
    fprintf(stderr, "pipe closed\n");
    exit(EXIT_FAILURE);
  } else if (ret == -1) {
    fprintf(stderr, "read failed\n");
    exit(EXIT_FAILURE);
  }
  ret = (atoi(strtok(buffer, " ")));
  if(ret != 1){
    size_t num_events = (size_t)(atoi(strtok(buffer, " ")));
    for (size_t i = 0; i < num_events; i++){
      ret = read(resp_pipe, buffer, TAMMSG - 1);
      if (ret == 0) {
        fprintf(stderr, "pipe closed\n");
        exit(EXIT_FAILURE);
      } else if (ret == -1) {
        fprintf(stderr, "read failed\n");
        exit(EXIT_FAILURE);
      }
      ret = write(out_fd, buffer, TAMMSG - 1);
      if (ret == -1) {
        fprintf(stderr, "write failed\n");
        exit(EXIT_FAILURE);
      }
    }
    //unsigned int *ids = strtok(buffer, " ");///memoria
  }

  return 1;
}
