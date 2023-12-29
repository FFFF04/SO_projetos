#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "common/io.h"
#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

typedef struct{
  int session_id;
  char mesg[84];
}data;


int S = MAX_SESSION_COUNT;
int active = 0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t show_lock = PTHREAD_MUTEX_INITIALIZER;


static void sig_handler(int sig) {
  if (sig == SIGINT) {
    if (signal(SIGINT, sig_handler) == SIG_ERR)
      exit(EXIT_FAILURE);
    set_to_show();
  }
}


void *threadfunction(void* arg){
  sigset_t new_mask;
  sigemptyset(&new_mask); // Initialize an empty signal set
  sigaddset(&new_mask, SIGUSR1); // Add SIGUSR1 to the signal set
  // Block SIGUSR1 in this thread
  pthread_sigmask(SIG_BLOCK, &new_mask, NULL);

  data *valores = (data*) arg;
  int op = atoi(strtok(valores->mesg, " "));
  char* req_pipe_name = strtok(NULL, " ");
  char* resp_pipe_name = strtok(NULL, " ");
  int freq = open(req_pipe_name, O_RDONLY);
  if (freq == -1){
    fprintf(stderr, "Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  int fresp = open(resp_pipe_name, O_WRONLY);
  if (fresp == -1){
    fprintf(stderr, "Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  if(op == 1){
    char buffer[16] = {};
    sprintf(buffer, "%d", valores->session_id);
    ssize_t ret = write(fresp, buffer, strlen(buffer) + 1);
    
    if (ret < 0) {
      fprintf(stderr, "Write failed\n");
      exit(EXIT_FAILURE);
    }
    op = 0;
  }
  while (1){
    unsigned int event_id;
    char buffer[TAMMSG];
    if(get_to_show()){
      if (pthread_mutex_lock(&g_mutex) != 0) {
          exit(EXIT_FAILURE);
      }
      active--;
      pthread_cond_signal(&cond);
      if (pthread_mutex_unlock(&g_mutex) != 0) {
        exit(EXIT_FAILURE);
      }
      break;
    }
    ssize_t ret = read(freq, buffer, TAMMSG);
    if (ret == -1) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
    }
    if (buffer[0] == 0)
      continue;
    
    int code_number = atoi(strtok(buffer, " "));
    // if(code_number>2 &&code_number < 6){
    //   event_id = (unsigned int)(atoi(strtok(NULL, " ")));
    // }
    buffer[ret] = 0;
    ssize_t escreve;
    switch (code_number) {
      case 2:
        if (pthread_mutex_lock(&g_mutex) != 0) {
          exit(EXIT_FAILURE);
        }
        active--;
        pthread_cond_signal(&cond);
        if (pthread_mutex_unlock(&g_mutex) != 0) {
          exit(EXIT_FAILURE);
        }
        return NULL;
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
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
        for (size_t i = 0; i < num_coords; i++) {
          xs[i] = (size_t)atoi(strtok(NULL, " "));
          ys[i] = (size_t)atoi(strtok(NULL, " "));
        }

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
        ems_show(fresp, event_id);
        //fprintf(stderr, "Failed to show event\n");
        break;
      case 6:
        ems_list_events(fresp);
          //fprintf(stderr, "Failed to list events\n");
        break;
    }
    memset(buffer, 0, sizeof(char) * TAMMSG);
  }
  return NULL;
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
  pipe_name = argv[1];
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
  data clients[S];
  pthread_t thread_id[S];
  int i = 0;
  pthread_mutex_t fifo_lock = getlock();
  while (1) {
    //TODO: Read from pipe
    if (signal(SIGINT, sig_handler) == SIG_ERR)
      exit(EXIT_FAILURE);//CTRL-C
    if(get_to_show()) 
      break;
    char *buffer = (char*) malloc(TAMMSG);
    memset(buffer, 0, TAMMSG);
    ssize_t ret = read(fserv, buffer, TAMMSG - 1);
    if (ret == -1) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
    }
    buffer[ret] = 0;
    if (buffer[0] != 0){
      if (pthread_mutex_lock(&g_mutex) != 0) {exit(EXIT_FAILURE);}
      while (active == S){
        pthread_cond_wait(&cond, &g_mutex);
      }
      memset(clients[i].mesg,0,strlen(clients[i].mesg)); 
      strncpy(clients[i].mesg, buffer,strlen(buffer) + 1);
      pthread_mutex_unlock(&fifo_lock);
      clients[i].session_id = i;
      if (pthread_create(&thread_id[i], NULL, &threadfunction, &clients[i]) != 0){
        fprintf(stderr, "Failed to create thread\n");
        exit(EXIT_FAILURE);
      }
      active++;
      if (pthread_mutex_unlock(&g_mutex) != 0) {
        exit(EXIT_FAILURE);
      }
      if(pthread_detach(thread_id[i]) != 0){
        fprintf(stderr, "Failed to detach thread\n");
        exit(EXIT_FAILURE);
      }
      i = (i+1) % S;
    }
    //TODO: Write new client to the producer-consumer buffer
    free(buffer);
  }

  ems_show_all(STDOUT_FILENO);
  ems_terminate();

  //TODO: Close Server
  close(fserv);
  unlink(pipe_name);
}