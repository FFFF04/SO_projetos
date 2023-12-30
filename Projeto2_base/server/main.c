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
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t sessions = PTHREAD_COND_INITIALIZER;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;


static void sig_handler(int sig) {
  if (sig == SIGINT) {
    if (signal(SIGINT, sig_handler) == SIG_ERR)
      exit(EXIT_FAILURE);
    set_to_show();
  }
}

//FALTA DA ERROS
void *threadfunction(void* arg){
  char* req_pipe_name;
  char* resp_pipe_name;
  int fresp , freq;
  sigset_t new_mask;
  sigemptyset(&new_mask); // Initialize an empty signal set
  sigaddset(&new_mask, SIGUSR1); // Add SIGUSR1 to the signal set
  // Block SIGUSR1 in this thread
  pthread_sigmask(SIG_BLOCK, &new_mask, NULL);

  if(pthread_detach(pthread_self()) != 0){
    fprintf(stderr, "Failed to detach thread\n");
    exit(EXIT_FAILURE);
  }

  data *valores = (data*) arg;
  while(enter_session != valores->session_id){
    if(get_to_show()) 
      return NULL;
    pthread_cond_wait(&sessions, &valores->session_lock);
  }
  printf("%d\n",valores->session_id);
  int op = atoi(strtok(valores->mensagem, " "));
  req_pipe_name = strtok(NULL, " "); 
  printf("%s\n",req_pipe_name);
  resp_pipe_name = strtok(NULL, " ");
  printf("%s\n",resp_pipe_name);
  freq = open(req_pipe_name, O_RDONLY);
  fresp = open(resp_pipe_name, O_WRONLY);
  if (freq == -1 || fresp == -1){
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
    memset(buffer, 0, TAMMSG);
  }
  return NULL;
}






void read_msg(int file, size_t size) {
  size_t reads = 0;
  char msg[84] = {};
  // char buff[1] = {};
  while (reads < size) {
    ssize_t ret = read(file, msg - reads, size - reads);
    if (ret < 0) {
      fprintf(stderr, "Read failed\n");
      exit(EXIT_FAILURE);
    }
    reads += (size_t)(ret);
  }
  strcpy(prod_consumidor,msg);
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

  fserv = open(pipe_name, O_RDONLY);
  if (fserv == -1) {
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }
  data clients[S];
  pthread_t thread_id[S];
  prod_consumidor = (char*) malloc(84);
  // memset(prod_consumidor, 0, 84);
  for (int k = 0; k < S; k++){
    clients[k].session_id = k;
    pthread_mutex_init(&clients[k].session_lock, NULL); ///DAR ERRO
    pthread_mutex_lock(&clients[k].session_lock);
    if (pthread_create(&thread_id[k], NULL, &threadfunction, &clients[k]) != 0){
      fprintf(stderr, "Failed to create thread\n");
      exit(EXIT_FAILURE);
    }
  }
  while (1) {
    //TODO: Read from pipe
    if (signal(SIGINT, sig_handler) == SIG_ERR)
      exit(EXIT_FAILURE);//CTRL-C
    if(get_to_show()) 
      break;
    
    read_msg(fserv,83);
    // ssize_t ret = read(fserv, prod_consumidor,TAMMSG);
    // if (ret == -1) {
    //   fprintf(stderr, "Read failed\n");
    //   exit(EXIT_FAILURE);
    // }
    
    if (prod_consumidor[0] != 0){
      
      if (pthread_mutex_lock(&g_mutex) != 0) 
        exit(EXIT_FAILURE);
      
      while (active == S){
        if(get_to_show()) 
          break;
        pthread_cond_wait(&cond, &g_mutex);
      }
      if (pthread_mutex_lock(&sessions_mutex) != 0) 
        exit(EXIT_FAILURE);
      enter_session = (enter_session + 1) % S;
      printf("enter_session: %d\n",enter_session);
      clients[enter_session].mensagem = (char *) malloc(84);
      strcpy(clients[enter_session].mensagem,prod_consumidor);
      printf("main: %s\n",clients[enter_session].mensagem);
      pthread_cond_broadcast(&sessions);
      if (pthread_mutex_unlock(&sessions_mutex) != 0) 
        exit(EXIT_FAILURE);
      
      active++;
      if (pthread_mutex_unlock(&g_mutex) != 0) {
        exit(EXIT_FAILURE);
      }
      memset(prod_consumidor, 0, 84);
    }
    //TODO: Write new client to the producer-consumer buffer
  }
  ems_show_all(STDOUT_FILENO);
  ems_terminate();
  free(prod_consumidor);
  //TODO: Close Server
  close(fserv);
  unlink(pipe_name);
}
//  while (true) {
//         char buffer[BUFFER_SIZE];
//         ssize_t ret = read(rx, buffer, BUFFER_SIZE - 1);
//         if (ret == 0) {
//             // ret == 0 signals EOF
//             fprintf(stderr, "[INFO]: pipe closed\n");
//             break;
//         } else if (ret == -1) {
//             // ret == -1 signals error
//             fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
//             exit(EXIT_FAILURE);
//         }

//         fprintf(stderr, "[INFO]: received %zd B\n", ret);
//         buffer[ret] = 0;
//         fputs(buffer, stdout);
//         send_msg(tx, "GAWK\n");
//     }