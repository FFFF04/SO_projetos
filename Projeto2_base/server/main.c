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
#include "operations.h"


typedef struct{
  int session_id;
  char *mensagem;
  pthread_mutex_t session_lock;
}data;

char *prod_consumidor;
int S = MAX_SESSION_COUNT;
int active = 0;
int enter_session = -1;
int fserv;
int *arr;
int size_arr;
int show_all=0;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t sessions = PTHREAD_COND_INITIALIZER;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t arr_lock = PTHREAD_MUTEX_INITIALIZER;


int del(){
  int valor = arr[1];
  for (int i = 2; i < size_arr; i++)
    arr[i - 1] = arr[i];
  arr = realloc(arr, (size_t)(size_arr - 1) * sizeof(int));
  if(arr != NULL)
    size_arr--;
  else {
    fprintf(stderr, "Memory reallocation failed. Deleting element aborted.\n");
    exit(EXIT_FAILURE);
  }
  return valor;
}


void add(int number){
  size_arr++;
  arr = realloc(arr, (size_t)(size_arr) * sizeof(int));
  if(arr != NULL)
    arr[size_arr - 1] = number;
  else {
    fprintf(stderr, "Memory reallocation failed. Adding element aborted.\n");
    exit(EXIT_FAILURE);
  }
}


static void sig_handler(int sig) {
  if (sig == SIGUSR1){
    set_to_show();
    show_all = 1;

  }
}


void *threadfunction(void* arg){
  char* req_pipe_name;
  char* resp_pipe_name;
  int fresp , freq;
  
  sigset_t new_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGUSR1);
  pthread_sigmask(SIG_BLOCK, &new_mask, NULL);

  if(pthread_detach(pthread_self()) != 0){
    fprintf(stderr, "Failed to detach thread\n");
    exit(EXIT_FAILURE);
  }

  data *valores = (data*) arg;
  while(enter_session != valores->session_id){
    pthread_cond_wait(&sessions, &valores->session_lock);
  }
  
  int op = atoi(strtok_r(valores->mensagem, " ", &valores->mensagem));
  req_pipe_name = strtok_r(valores->mensagem, " ", &valores->mensagem); 
  resp_pipe_name = strtok_r(valores->mensagem, " ", &valores->mensagem);
  freq = open(req_pipe_name, O_RDONLY);
  fresp = open(resp_pipe_name, O_WRONLY);
  
  if (freq == -1 || fresp == -1){
    fprintf(stderr, "Pipe open failed\n");
    exit(EXIT_FAILURE);
  }
  if (pthread_mutex_unlock(&sessions_mutex) != 0) 
    exit(EXIT_FAILURE);
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
    int return_value;
    int code_number;
    ssize_t ret = read(freq, &code_number, sizeof(code_number));
    if (ret == -1) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
    }
    switch (code_number) {
      case 2:
        if (unlink(req_pipe_name) == -1 || unlink(resp_pipe_name) == -1) {
          fprintf(stderr,"Error unlinking pipe");
          exit(EXIT_FAILURE);
        }
        if (pthread_mutex_lock(&arr_lock) != 0) {
          exit(EXIT_FAILURE);
        }
        add(valores->session_id);
        if (pthread_mutex_unlock(&arr_lock) != 0) {
          exit(EXIT_FAILURE);
        }
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
        size_t num_rows, num_columns;
        if(read(freq, &event_id, sizeof(unsigned int)) < 0 || read(freq, &num_rows, sizeof(size_t)) < 0 
          || read(freq, &num_columns, sizeof(size_t)) < 0){
          fprintf(stderr, "Read failed\n");
          exit(EXIT_FAILURE);
        }
        return_value = ems_create(event_id, num_rows, num_columns);
        if(write(fresp,&return_value, sizeof(int)) < 0){
          fprintf(stderr, "Write failed\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 4:
        size_t num_coords;
        size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
        if(read(freq, &event_id, sizeof(unsigned int)) < 0 || read(freq, &num_coords, sizeof(size_t)) < 0 ||
          read(freq, &xs, sizeof(size_t) * MAX_RESERVATION_SIZE) < 0 
          || read(freq, &ys, sizeof(size_t) * MAX_RESERVATION_SIZE) < 0){
          fprintf(stderr, "Read failed\n");
          exit(EXIT_FAILURE);
        }
        return_value = ems_reserve(event_id, num_coords, xs, ys);
        if(write(fresp,&return_value, sizeof(int)) < 0){
          fprintf(stderr, "Write failed\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 5:
        if(read(freq, &event_id, sizeof(unsigned int)) < 0){
          fprintf(stderr, "Read failed\n");
          exit(EXIT_FAILURE);
        }
        ems_show(fresp, event_id);
        break;
      case 6:
        ems_list_events(fresp);
        break;
    }
  }
  if (pthread_mutex_lock(&arr_lock) != 0) {
    exit(EXIT_FAILURE);
  }
  add(valores->session_id);
  if (pthread_mutex_unlock(&arr_lock) != 0) {
    exit(EXIT_FAILURE);
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

  fserv = open(pipe_name, O_RDWR);
  if (fserv == -1) {
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }
  data clients[S];
  pthread_t thread_id[S];
  prod_consumidor = (char*) malloc(84);
  if (prod_consumidor == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  arr = (int*) malloc(sizeof(int) * (size_t)(S+1));
  if (arr == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  size_arr = S+1;
  for (int k = 0; k < S; k++){
    arr[k+1] = k;
    clients[k].session_id = k;
    if(pthread_mutex_init(&clients[k].session_lock, NULL)!= 0){
      fprintf(stderr, "Mutex initialization failed \n");
      exit(EXIT_FAILURE);
    } 
    pthread_mutex_lock(&clients[k].session_lock);
    if (pthread_create(&thread_id[k], NULL, &threadfunction, &clients[k]) != 0){
      fprintf(stderr, "Failed to create thread\n");
      exit(EXIT_FAILURE);
    }
  }
  struct sigaction s;
  s.sa_handler = &sig_handler;
  sigemptyset(&s.sa_mask);
  if (sigaction(SIGUSR1, &s, NULL) == -1){
    fprintf(stderr, "Error sigaction\n");
    exit(EXIT_FAILURE);
  } 
  int cria_threads = 0;
  while (1) {
    if(show_all == 1){
      ems_show_all(STDOUT_FILENO);
      reset_to_show();
      show_all = 0;
    }

    read_msg(prod_consumidor,fserv,83);
    
    if (prod_consumidor[0] != 0){
      
      if (pthread_mutex_lock(&g_mutex) != 0) 
        exit(EXIT_FAILURE);
      while (active == S){
        pthread_cond_wait(&cond, &g_mutex);
      }

      if (pthread_mutex_lock(&arr_lock) != 0) {
        exit(EXIT_FAILURE);
      }
      int index = del();
      if (pthread_mutex_unlock(&arr_lock) != 0) {
        exit(EXIT_FAILURE);
      }
      clients[index].mensagem = (char *) malloc(84);
      if (clients[index].mensagem  == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }
      memcpy(clients[index].mensagem, prod_consumidor, strlen(prod_consumidor)+1);
      if(cria_threads == 1){
        clients[index].session_id = index;
        pthread_mutex_init(&clients[index].session_lock, NULL); 
        pthread_mutex_lock(&clients[index].session_lock);
        if (pthread_create(&thread_id[index], NULL, &threadfunction, &clients[index]) != 0){
          fprintf(stderr, "Failed to create thread\n");
          exit(EXIT_FAILURE);
        }
      }

      if (pthread_mutex_lock(&sessions_mutex) != 0)
        exit(EXIT_FAILURE);

      enter_session = index;
      if(enter_session == S-1)
        cria_threads = 1;
      pthread_cond_broadcast(&sessions);
      active++;
      if (pthread_mutex_unlock(&g_mutex) != 0) {
        exit(EXIT_FAILURE);
      }
      memset(prod_consumidor, 0, 84);
    }
    //TODO: Write new client to the producer-consumer buffer
  }
  for (int k = 0; k < S; k++){
    if (pthread_mutex_destroy(&clients[k].session_lock) != 0){
      fprintf(stderr, "Failed to destroy lock\n");
      exit(EXIT_FAILURE);
    }
  }
  if (pthread_mutex_destroy(&g_mutex) != 0 || pthread_mutex_destroy(&read_lock) != 0 
    || pthread_mutex_destroy(&sessions_mutex) != 0 || pthread_mutex_destroy(&arr_lock) ||
      pthread_cond_destroy(&cond) != 0 || pthread_cond_destroy(&sessions) != 0){
    fprintf(stderr, "Failed to destroy lock\n");
    exit(EXIT_FAILURE);
  }

  ems_terminate();
  free(prod_consumidor);
  //TODO: Close Server
  if (close(fserv) == -1) {
    fprintf(stderr, "Error closing resp_pipe");
    exit(EXIT_FAILURE);
  }
  if (unlink(pipe_name) == -1) {
    fprintf(stderr,"Error unlinking pipe_name");
    exit(EXIT_FAILURE);
  }
}