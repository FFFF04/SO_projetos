#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#define TAMMSG 1000

void *threadfunction(int op, char *req_pipe_name, char *resp_pipe_name){
  int freq = open(req_pipe_name, O_RDONLY);
  if (freq == -1){
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }
  int fresp = open(resp_pipe_name, O_WRONLY);
  if (fresp == -1){
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }
  
  if(op == 1){
    int session_id = 0;
    char *str = (char*) malloc(sizeof(char)*16);
    sprintf(str,"%u",session_id);
    ssize_t ret = write(fresp, str, sizeof(str));
    if (ret < 0) {
      fprintf(stderr, "Write failed\n");
      exit(EXIT_FAILURE);
    }
    op = 0;
    free(str);
  }

  while (1){
    char buffer[10];
    ssize_t ret = read(freq, buffer, 10 - 1);
    if (ret == 0) {
      fprintf(stderr, "pipe closed\n");
      exit(EXIT_FAILURE);
    } else if (ret == -1) {
      fprintf(stderr, "read failed\n");
      exit(EXIT_FAILURE);
    }
    int code_number = atoi(strtok(buffer, " "));
    buffer[ret] = 0;
    unsigned int event_id;
    long int escreve;
    int fclient;
    switch (code_number) {
      case 2:
        ems_terminate();
        close(freq);
        close(fresp);
        unlink(req_pipe_name);
        unlink(resp_pipe_name);
        exit(EXIT_SUCCESS);
      case 3:
        event_id = (unsigned int)(atoi(strtok(NULL, " ")));
        size_t num_rows = (size_t)(atoi(strtok(NULL, " ")));
        size_t num_columns = (size_t)(atoi(strtok(NULL, " ")));
        if(ems_create(event_id, num_rows, num_columns))
          escreve = write(fresp,"1\n",2);
        else
          escreve = write(fresp,"0\n",2);
        if (escreve < 0) {
          fprintf(stderr, "Error writing in pipe\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 4:
        event_id = (unsigned int)(atoi(strtok(NULL, " ")));
        size_t num_coords = (size_t)(atoi(strtok(NULL, " ")));
        size_t* xs = (size_t*)(atoi(strtok(NULL, " ")));///memoria
        size_t* ys = (size_t*)(atoi(strtok(NULL, " ")));///memoria
        if(ems_reserve(event_id, num_coords, xs, ys))
          escreve = write(fresp,"1\n",2);
        else
          escreve = write(fresp,"0\n",2);
        if (escreve < 0) {
          fprintf(stderr, "Error writing in pipe\n");
          exit(EXIT_FAILURE);
        }
        // fprintf(stderr, "Failed to create event\n");
        break;
      case 5:
        event_id = (unsigned int)(atoi(strtok(NULL, " ")));
        escreve = write(fresp,"0\n",2);
        if (escreve < 0) {
          fprintf(stderr, "Error writing in pipe\n");
          exit(EXIT_FAILURE);
        }
        if(ems_show(fresp, event_id)){
          unlink(resp_pipe_name);
          if (unlink(resp_pipe_name) != 0 && errno != ENOENT) {
            fprintf(stderr, "unlink failed\n");
            exit(EXIT_FAILURE);
          }
          if (mkfifo(resp_pipe_name, 0777) < 0){
            fprintf(stderr, "mkfifo failed\n");
            exit(EXIT_FAILURE);
          }
          fclient = open(resp_pipe_name, O_RDONLY);
          if (fclient == -1) {
            fprintf(stderr, "Pipe open failed\n");
            exit(EXIT_FAILURE);
          }
          escreve = write(fresp,"1\n",2);
          if (escreve < 0) {
            fprintf(stderr, "Error writing in pipe\n");
            exit(EXIT_FAILURE);
          }
        }
        //fprintf(stderr, "Failed to show event\n");
        break;
      case 6:
        escreve = write(fresp,"0\n",2);
        if (escreve < 0) {
          fprintf(stderr, "Error writing in pipe\n");
          exit(EXIT_FAILURE);
        }
        if(ems_list_events(fresp)){
          unlink(resp_pipe_name);
          if (unlink(resp_pipe_name) != 0 && errno != ENOENT) {
            fprintf(stderr, "unlink failed\n");
            exit(EXIT_FAILURE);
          }
          if (mkfifo(resp_pipe_name, 0777) < 0){
            fprintf(stderr, "mkfifo failed\n");
            exit(EXIT_FAILURE);
          }
          fclient = open(resp_pipe_name, O_RDONLY);
          if (fclient == -1) {
            fprintf(stderr, "Pipe open failed\n");
            exit(EXIT_FAILURE);
          }
          escreve = write(fresp,"1\n",2);
          if (escreve < 0) {
            fprintf(stderr, "Error writing in pipe\n");
            exit(EXIT_FAILURE);
          }
        }
          //fprintf(stderr, "Failed to list events\n");
        break;
    }
  }
  exit(EXIT_SUCCESS);
}


int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }
  char *pipe_name = "";
  char* endptr;
  int fserv;

  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  //TODO: Intialize server, create worker threads
  /*
  COM ISTO JA TEMOS O SERVIDOR INICIALIZADO FALTA AS WORKER THREADS
  PARA ISSO ACHO QUE TEMOS DE:
    QUANDO QUERIAMOS UMA NOVO CLIENTE CRIASSE TAMBEM UMA THREAD NOVA QUE 
    FICA ASSOCIADA A ESSE CLINTE
  */
  pipe_name = argv[1];
  unlink(pipe_name);
  if (unlink(pipe_name) != 0 && errno != ENOENT) {
    fprintf(stderr, "unlink failed\n");
    exit(EXIT_FAILURE);
  }
  if (mkfifo(pipe_name, 0777) < 0){
    fprintf(stderr, "mkfifo failed\n");
    exit(EXIT_FAILURE);
  } 

  fserv = open(pipe_name, O_RDONLY);
  if (fserv == -1) {
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    /*OK JA PERCEBI NO API NOS CRIAMOS O PIPE DO CLINTE
    DENTRO DA FUNCAO DO EMS_SETUP VAI ESCREVER NO PIPE_NAME 
    DESTE LADO LEMOS MEIO QUE INSCREVEMOS O DUDE NO SERVER */
    //TODO: Read from pipe
    char buffer[TAMMSG];
    ssize_t ret = read(fserv, buffer, TAMMSG - 1);
    if (ret == 0) {
      fprintf(stderr, "Pipe closed\n");
      exit(EXIT_SUCCESS);
    } else if (ret == -1) {
        fprintf(stderr, "Read failed\n");
        exit(EXIT_FAILURE);
    }
    buffer[ret] = 0;
    int op = atoi(strtok(buffer, " "));
    char* req_pipe_name = strtok(NULL, " ");
    char* resp_pipe_name = strtok(NULL, " ");
    threadfunction(op,req_pipe_name,resp_pipe_name);
    
    //TODO: Write new client to the producer-consumer buffer
  }
  /*QUANDO O SERVIDOR ESTA CHEIO ENTAO FAZEMOS PTHREAD_WAIT QUE IRA FAZER ESPERAR ATE QUE UM
  CLIENTE SAIA DO SERVIDOR, QUANDO UM CLIENTE SAI ENTAO FAZEMOS SIGNEL PARA PODER ENTRAR OUTRO BACANO*/

  //TODO: Close Server
  close(fserv);
  unlink(pipe_name);
  ems_terminate();
}