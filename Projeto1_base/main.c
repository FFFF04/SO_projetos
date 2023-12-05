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
  /*char *path = "/home/francisco/SO/SO_projeto_1/Projeto1_base/jobs";
  int max_proc = 1;
  int max_threads = 2;*/
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
    //PERGUNTAR AO PROF
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
      unsigned long int delay = (unsigned int) strtoul(argv[i], &endptr, 10);///verificar e pq 2 delays

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
        wait(NULL);
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
        printf("%d\n",getpid());
        int file = open_file_read(path,dp->d_name);
        int file_out = open_file_out(path, dp->d_name);
        /*pthread_t thread_id[max_threads];
        int j = 0;
        int paragem = 0;
        data* valores;
        while (1){
          for (int i = 0; i < max_threads; i++){ 
            valores = (data*) malloc(sizeof(data));
            
            if ((valores->command = get_next(valores->file)) >= 0){
              paragem = valores->command;
              valores->file = file;
              valores->file_out = file_out;
              j++;
              if (pthread_create(&thread_id[i],NULL,&process,valores) != 0){
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
              }
              if (valores->command == EOC) break;
              
            }
          }
          for (int k = 0; k < j; k++){
            if (pthread_join(thread_id[k],NULL) != 0){
              fprintf(stderr, "Failed to join thread\n");
              exit(EXIT_FAILURE);
            }
            free(valores);
          }
          j = 0;
          if (paragem == EOC) break;
        }*/
        int current_thread = 0;
        int i = 0;
        //pthread_t thread_id[100];////alterar indice que controla
        int comando = 0;
        while ((comando = get_next(file)) >= 0){
          printf("%d\n",comando);
          //pthread_t thread_id[i];
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
          valores->command = comando;
          
          if (pthread_create(&thread_id[i],NULL,&process,valores) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
          }
          current_thread++;
          i++;
          if (valores->command == EOC) break;
          free(valores);
        }
        i--;
        while(current_thread >= 0){
          if (pthread_join(thread_id[i - current_thread], NULL) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
          }
          current_thread--;
        }
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