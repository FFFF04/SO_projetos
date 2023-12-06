#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "files.h"
#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "process.h"

pthread_mutex_t lock;
int comando;

void* process(void* arg){
    data *valores = (data*) arg;
    unsigned int event_id;
    unsigned int delay;
    size_t num_rows;
    size_t num_columns;
    size_t num_coords;
    size_t xs[MAX_RESERVATION_SIZE];
    size_t ys[MAX_RESERVATION_SIZE];
    if ((comando = get_next(valores->file)) < 0)
    {
       /*Da ERRO*/
    }
    //int* command = malloc(sizeof(int));
    //command = &valores->command;
    //printf("valores.command dentro: %d\n", valores->command);
    if (pthread_mutex_init(&lock, NULL) != 0) {
       fprintf(stderr, "Failed to initialize the lock\n");
       exit(EXIT_FAILURE);
    }
    switch (comando) {
    case CMD_CREATE:
        //printf("Create : %d\n", valores->command);
        if (parse_create(valores->file, &event_id, &num_rows, &num_columns) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //free(arg);
            return NULL;
        }
        if (ems_create(event_id, num_rows, num_columns)) {
            fprintf(stderr, "Failed to create event\n");
        }
        //free(arg);
        return NULL;
    }
    pthread_mutex_lock(&lock);
    switch (comando) {
    case CMD_RESERVE:
        
        num_coords = parse_reserve(valores->file, MAX_RESERVATION_SIZE, &event_id,xs,ys);
        if (num_coords == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //free(arg);
            return NULL;
        }
        if (ems_reserve(event_id, num_coords, xs,ys)) {
            fprintf(stderr, "Failed to reserve seats\n");
        }
        break;
    case CMD_SHOW:
        if (parse_show(valores->file, &event_id) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //free(arg);
            return NULL;
        }
        if (ems_show(valores->file_out, event_id)) {
            fprintf(stderr, "Failed to show event\n");
        }
        break;
    case CMD_LIST_EVENTS:
        if (ems_list_events()) {
            fprintf(stderr, "Failed to list events\n");
        }
        break;
    case CMD_WAIT:
        //pid_t thread_id = gettid();
        if (parse_wait(valores->file, &delay, NULL/*&thread_id*/) == -1) {  // thread_id is not implemented
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //free(arg);
            return NULL;
        }
        if (delay > 0) {
            printf("Waiting...\n");
            ems_wait(delay);
        }
        break;
    case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;
    case CMD_HELP:
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");
        break;
    case CMD_BARRIER:  // Not implemented
    case CMD_EMPTY:
      break;
    case EOC:
        ems_terminate();
        if (close(valores->file) == 1){
            write(STDERR_FILENO, "Error closing file\n", 20);
            exit(EXIT_FAILURE);
        }
        if (close(valores->file_out) == 1){
            write(STDERR_FILENO, "Error closing file\n", 20);
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutex_unlock(&lock);
    //free(arg);
    return NULL;
}


void read_files(char* path, char* name, int max_threads){
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    pthread_t thread_id[max_threads];
    data valores[max_threads];
    int j = 0;
    while (1){
      for (int i = 0; i < max_threads; i++){ 
          j++;
          valores[i].file = file;
          valores[i].file_out = file_out;
          if (pthread_create(&thread_id[i],NULL,&process,&valores[i]) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
          }
          if (comando == EOC) break;
      }
      for (int k = 0; k < j; k++){
        if (pthread_join(thread_id[k],NULL) != 0){
          fprintf(stderr, "Failed to join thread\n");
          exit(EXIT_FAILURE);
        }
      }
      j = 0;
      //printf("paragem:%d\n",paragem);
      //fflush(stdout);
      if (comando == EOC) break;
    }
    /*int current_thread = 0;
    int i = 0;
    pthread_t thread_id[100];////alterar indice que controla
    while (1){
      //printf("%d\n",comando);
      //pthread_t thread_id[i];
      //void* return_value;
      if(current_thread >= max_threads){
        if (pthread_join(thread_id[i - current_thread], NULL) != 0){
          fprintf(stderr, "Failed to create thread\n");
          exit(EXIT_FAILURE);
        }
        current_thread--;
      }
      data* valores = (data*) malloc(sizeof(data));
      valores->file = file;
      valores->file_out = file_out;
      //valores->command = comando;
      
      if (pthread_create(&thread_id[i], NULL, &process, valores) != 0){
        fprintf(stderr, "Failed to create thread\n");
        exit(EXIT_FAILURE);
      }
      current_thread++;
      i++;
      //int* comando = (int*)return_value;
      printf("%d\n",valores->command);
      fflush(stdout);
      if (valores->command == EOC) break;
      //free(comando);
      //free(valores);
    }
    i--;
    while(current_thread >= 0){
      if (pthread_join(thread_id[i - current_thread], NULL) != 0){
        fprintf(stderr, "Failed to create thread\n");
        exit(EXIT_FAILURE);
      }
      current_thread--;
    }*/
}