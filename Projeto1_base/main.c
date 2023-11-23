#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 1) {
    char *endptr;
    unsigned long int delay = strtoul(argv[1], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  while (1) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    //char *buffer = (char*) malloc(sizeof(char)*100);
    char buffer[100] = {};
    int input = read(STDIN_FILENO, buffer,100 - 1);
    if (input == -1){
        errExit("Error reading path");
    }
    
    buffer[strlen(buffer)-1] = '\0';

    int file = open(buffer,O_RDONLY);
    if (file == -1) {
        errExit("Error opening file");
    }
    int file_in_size = strlen(buffer)- strlen("jobs\0");
    char *file_out_name = (char*) malloc(sizeof(char) * strlen(buffer));
    strncpy(file_out_name, buffer, file_in_size);
    strcat(file_out_name, "out\0");
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
      S_IROTH | S_IWOTH;
    int file_out = open(file_out_name, O_CREAT | O_TRUNC | O_WRONLY , filePerms);
    if (file_out == -1) {
        errExit("Error opening file");
    }
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
          return 0;
      }
    }
    if (close(file) == -1)
      errExit("close input");
    if (close(file) == -1)
      errExit("close input");
  }
}
