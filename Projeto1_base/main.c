#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>

#include "constants.h"
#include "operations.h"
#include "files.h"
#include "process.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char *path = "";
  int max_proc = 0;
  int max_threads = 0;
  DIR *dirp;
  struct dirent *dp;
  if (argc <= 3){
    fprintf(stderr, "To few arguments\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 1; i < argc; i++){
    if(i == 1){
      dirp = opendir(argv[i]);
      if (dirp == NULL) {
        fprintf(stderr, "Error opening Dirpath\n");
        exit(EXIT_FAILURE);
      }
      path = argv[i];
    }
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
      unsigned long int delay = (unsigned int) strtoul(argv[i], &endptr, 10);

      if (*endptr != '\0' || delay > UINT_MAX) {
        fprintf(stderr, "Invalid delay value or value too large\n");
        return 1;
      }

      state_access_delay_ms = (unsigned int)delay;
    }
  }
  while (1) {
    pid_t pid;
    int processos = 0;
    while ((dp = readdir(dirp)) != NULL){
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
          continue;
      char *out = strstr(dp->d_name, ".jobs");
      if (out == NULL || strcmp(out, ".jobs") != 0)
          continue;
        
      if (processos >= max_proc){
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
        read_files(path,dp->d_name,max_threads);
        exit(EXIT_SUCCESS);
      }
      else{
        processos++;
        continue;
      }  
    }
    break;
  }
  wait(NULL);
  closedir(dirp);
  return 0;
}