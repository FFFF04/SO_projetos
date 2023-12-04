#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"
#include "files.h"
#include "process.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char *path = "";
  int max_proc = 0;
  int max_threads = 0;
  unsigned int delay = 0;
  DIR *dirp;
  struct dirent *dp;
  if (argc < 3){
    /*DA ERRO */
  }
  
  for (int i = 1; i < argc; i++){
    if(i == 1){
      dirp = opendir(argv[i]);
      if (dirp == NULL) {
        fprintf(stderr, "EError opening Dirpath\n");
        exit(EXIT_FAILURE);
      }
      path = argv[i];
    }
    /*PERGUNTAR AO PROF*/
    if(i == 2){
      char* max_proc_char = argv[i];
      max_proc = atoi(max_proc_char);
      if (max_proc <= 0) {
        fprintf(stderr, "Error MAX_PROC not valid\n");
        exit(EXIT_FAILURE);
      }
    }
    if(i == 3){
      char* max_threads_char = argv[i];
      max_threads = atoi(max_threads_char);
      if (max_threads <= 0) {
        fprintf(stderr, "Error MAX_THREADS not valid\n");
        exit(EXIT_FAILURE);
      }
    }    
    if (i == 4){
      char *endptr;
      delay = (unsigned int) strtoul(argv[i], &endptr, 10);///verificar e pq 2 delays

      if (*endptr != '\0' || delay > UINT_MAX) {
        fprintf(stderr, "Invalid delay value or value too large\n");
        return 1;
      }

      state_access_delay_ms = (unsigned int)delay;
    }
  }

  while (1) {
    /*unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];*/
    pid_t pid;
    int processos = 0;
    while ((dp = readdir(dirp)) != NULL){
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
          continue;
      char *out = strstr(dp->d_name, ".out");
      if (out!=NULL && (strcmp(out, ".out") == 0))
        continue;
        
      if (processos >= max_proc){
        // Falta por o id? nao nem sei se é
        int p = wait(NULL);
        printf("%d\n",p);
        fflush(stdout);
        processos--;
      }
      pid = fork();

      if (pid == -1) {
        fprintf(stderr, "Failed to create a process\n");
        return 1;
      }
      if (pid == 0) {
        if (ems_init(state_access_delay_ms)) {
          fprintf(stderr, "Failed to initialize EMS\n");
          return 1;
        } 
        data* valores = (data*) malloc(sizeof(data));
        valores->file = open_file_read(path,dp->d_name);
        valores->file_out = open_file_out(path, dp->d_name);
        valores->delay = delay;
        fflush(stdout);
        //int current_thread = 0;
        pthread_t thread_id[max_threads];
        //int i = 0;
        int j = 0;
        while (1){          
          for (int i = 0; i < max_threads; i++)
          { 
            if ((valores->command = get_next(valores->file)) >= 0)
            {
              j++;
              if (pthread_create(&thread_id[i],NULL,&process,valores) != 0){
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
              }
            }
          }
          for (int k = 0; k < j; k++){
            if (pthread_join(thread_id[k],NULL) != 0){
              fprintf(stderr, "Failed to join thread\n");
              exit(EXIT_FAILURE);
            }
          }
          j = 0;
          if (valores->command == EOC) break;
        }
        free(valores);
        /*while ((valores->command = get_next(valores->file)) >= 0){
          pthread_t thread_id[i];
          if(current_thread >= max_threads){
            if (pthread_join(thread_id[i - current_thread], NULL) != 0){
              fprintf(stderr, "Failed to create thread\n");
              exit(EXIT_FAILURE);
            }
            current_thread--;
          }
          if (pthread_create(&thread_id[i],NULL,&process,valores) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
          }
          current_thread++;
          i++;
          if (valores->command == EOC) break;
        }
        i--;
        while(current_thread >= 0){
          if (pthread_join(thread_id[i - current_thread], NULL) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
          }
          current_thread--;
        }*/
        exit(EXIT_SUCCESS);
      }
      else{
        processos++;
        continue;
      }  
    }
    break;
  }
  closedir(dirp);
  return 0;
}