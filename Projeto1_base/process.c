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

void* process(void* arg){
    data valores = *(data*) arg;
    if (pthread_mutex_init(&lock, NULL) != 0) {
       fprintf(stderr, "Failed to initialize the lock\n");
       exit(EXIT_FAILURE);
    }
    switch (valores.command) {
    case CMD_CREATE:
        
        if (parse_create(valores.file, &valores.event_id, &valores.num_rows, &valores.num_columns) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            free(arg);
            return NULL;
        }
        if (ems_create(valores.event_id, valores.num_rows, valores.num_columns)) {
            fprintf(stderr, "Failed to create event\n");
        }
        free(arg);
        return NULL;
    }
    pthread_mutex_lock(&lock);
    switch (valores.command) {
    case CMD_RESERVE:
        
        valores.num_coords = parse_reserve(valores.file, MAX_RESERVATION_SIZE, &valores.event_id,valores.xs,valores.ys);
        if (valores.num_coords == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            free(arg);
            return NULL;
        }
        if (ems_reserve(valores.event_id, valores.num_coords, valores.xs,valores.ys)) {
            fprintf(stderr, "Failed to reserve seats\n");
        }
        break;
    case CMD_SHOW:
        if (parse_show(valores.file, &valores.event_id) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            free(arg);
            return NULL;
        }
        if (ems_show(valores.file_out, valores.event_id)) {
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
        if (parse_wait(valores.file, &valores.delay, NULL/*&thread_id*/) == -1) {  // thread_id is not implemented
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            free(arg);
            return NULL;
        }
        if (valores.delay > 0) {
            printf("Waiting...\n");
            ems_wait(valores.delay);
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
        if (close(valores.file) == -1){
            write(STDERR_FILENO, "Error closing file\n", 20);
            exit(EXIT_FAILURE);
        }
        if (close(valores.file_out) == -1){
            write(STDERR_FILENO, "Error closing file\n", 20);
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutex_unlock(&lock);
    free(arg);
    return NULL;
}