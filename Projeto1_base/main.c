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
#include "files.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
  char *path = "";
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
      int max_proc = atoi(max_proc_char);
      if (max_proc <= 0) {
        fprintf(stderr, "Error MAX_PROC not valid\n");
        exit(EXIT_FAILURE);
      }
    }
    if (i == 3){
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
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        continue;
      char *out = strstr(dp->d_name, ".out");
      if (out!=NULL && (strcmp(out, ".out") == 0))
        continue;
      if (ems_init(state_access_delay_ms)) {
        fprintf(stderr, "Failed to initialize EMS\n");
        return 1;
      } 
      int file = open_file_read(path,dp->d_name);
      int file_out = open_file_out(path, dp->d_name);
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
        if (i == EOC) break;
      }
    }
    break;
  }
  closedir(dirp);
  return 0;
}
