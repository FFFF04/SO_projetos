#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char *path = "";
  DIR *dirp;
  struct dirent *dp;
  if (argc == 1){
    /* DA ERRO */
  }
  
  for (int i = 1; i < argc; i++){
    if(i == 1){
      dirp = opendir(argv[i]);
      if (dirp == NULL) {
        write(STDERR_FILENO, "Error opening Dirpath\n", 22);
        exit(EXIT_FAILURE);
      }
      path = argv[i];
    }
    if (i == 2){
      char *endptr;
      unsigned long int delay = strtoul(argv[2], &endptr, 10);

      if (*endptr != '\0' || delay > UINT_MAX) {
        fprintf(stderr, "Invalid delay value or value too large\n");
        return 1;
      }

      state_access_delay_ms = (unsigned int)delay;
    }
  }

  
  while (1) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    while ((dp = readdir(dirp)) != NULL) {
      /*printf("ola\n");
      fflush(stdout);*/
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        continue;
      char *out = strstr(dp->d_name, ".out");
      if (out!=NULL && (strcmp(out, ".out") == 0))
        continue;
      if (ems_init(state_access_delay_ms)) {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
      } 
      //printf("%s\n",dp->d_name);//nao esta alfabeticamente ordenado
      char *filename = (char *) malloc(strlen(path) + strlen("/") + strlen(dp->d_name) + 1); //erro para alocar memoria
      strcpy(filename,path);
      strcat(filename,"/");
      strcat(filename,dp->d_name);
      
      // criar um cadeia de caracteres onde o .jobs e trocado por .out para escrever o output do ficheiro lido
      size_t replace_str_len = strlen("jobs");
      size_t replace_with_len = strlen("out");
      size_t modified_len = strlen(dp->d_name) - replace_str_len + replace_with_len;
      char *file_out_name = (char *)malloc((modified_len + 1) * sizeof(char));
      char *found_position = strstr(dp->d_name, "jobs");
      size_t copy_length = (size_t)(found_position - dp->d_name);
      strncpy(file_out_name, dp->d_name, copy_length);
      file_out_name[copy_length] = '\0';
      strcat(file_out_name, "out");
      char *filename_out = (char *) malloc(strlen(path) + strlen("/") + strlen(file_out_name) + 1);
      strcpy(filename_out,path);
      strcat(filename_out,"/");
      strcat(filename_out,file_out_name);
      int file = open(filename,O_RDONLY);
      if (file == -1) {
        write(STDERR_FILENO, "Error opening file\n", 19);
        exit(EXIT_FAILURE);
      }

      int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
      int file_out = open(filename_out, O_CREAT | O_TRUNC | O_WRONLY , filePerms);
      if (file_out == -1) {
        write(STDERR_FILENO, "Error opening file\n", 19);
        exit(EXIT_FAILURE);
      }
      free(filename);
      free(file_out_name);
      free(filename_out);
      fflush(stdout);
      int i;
      while ((i =  get_next(file)) >= 0){
          switch (i) {
          case CMD_CREATE:
            if (parse_create(file, &event_id, &num_rows, &num_columns) != 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              continue;
            }

            if (ems_create(event_id, num_rows, num_columns)) {
              fprintf(stderr, "Failed to create event\n");
            }

            break;

          case CMD_RESERVE:
            num_coords = parse_reserve(file, MAX_RESERVATION_SIZE, &event_id, xs, ys);

            if (num_coords == 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              continue;
            }

            if (ems_reserve(event_id, num_coords, xs, ys)) {
              fprintf(stderr, "Failed to reserve seats\n");
            }

            break;

          case CMD_SHOW:
            if (parse_show(file, &event_id) != 0) {
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              continue;
            }

            if (ems_show(file_out, event_id)) {
              fprintf(stderr, "Failed to show event\n");
            }

            break;

          case CMD_LIST_EVENTS:
            if (ems_list_events()) {
              fprintf(stderr, "Failed to list events\n");
            }

            break;

          case CMD_WAIT:
            if (parse_wait(file, &delay, NULL) == -1) {  // thread_id is not implemented
              fprintf(stderr, "Invalid command. See HELP for usage\n");
              continue;
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
            if (close(file) == -1){
            write(STDERR_FILENO, "Error closing file\n", 20);
            exit(EXIT_FAILURE);
            }

            if (close(file_out) == -1){
              write(STDERR_FILENO, "Error closing file\n", 20);
              exit(EXIT_FAILURE);
            }
            //return 0;
        }
        if (i == 9) break;
      }
    }
    break;
  }
  closedir(dirp);
  return 0;
}
