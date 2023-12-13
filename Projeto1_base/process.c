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

int comando;
int barrier = 0; //0 nao ha barrier 1 ha barrier
int max_Threads;
waiting_list *wait_list;
pthread_mutex_t read_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_file_lock = PTHREAD_MUTEX_INITIALIZER;

void del(int n, unsigned int *arr){
    for (int i = 1; i < n; i++)
        arr[i - 1] = arr[i];
    arr = realloc(arr, (size_t)(n - 1) * sizeof(int));
}

void* thread(void* arg){
    data *valores = (data*) arg;
    unsigned int event_id, delay ,thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int *return_value;
    int value;
    return_value = &value;
    while (comando != EOC){
        if(valores->thread_id == 1)
            printf("thread_1\n");
        if(valores->thread_id == 2)
            printf("thread_2\n");
        if (wait_list[valores->thread_id].size != 0){
            long int escreve = write(valores->file_out,"Waiting...\n",11);
            if (escreve < 0) {
                fprintf(stderr, "Error writing in file\n");
                exit(EXIT_FAILURE);
            }
            ems_wait(wait_list[valores->thread_id].waiting_time[0]);
            if(wait_list[valores->thread_id].size>1)
                del(wait_list[valores->thread_id].size, wait_list[valores->thread_id].waiting_time);
            wait_list[valores->thread_id].size--;
        }
        pthread_mutex_lock(&read_file_lock);
        if (barrier == 1){
            return_value = &barrier;
            pthread_mutex_unlock(&read_file_lock);
            break;
        } 
        int command = get_next(valores->file);
        //pthread_mutex_lock(&write_file_lock);
        switch (command) {
        case CMD_CREATE:
            if (parse_create(valores->file, &event_id, &num_rows, &num_columns) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                break;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_RESERVE:
            num_coords = parse_reserve(valores->file, MAX_RESERVATION_SIZE, &event_id,xs,ys);
            if (num_coords == 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                break;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            //pthread_mutex_unlock(&read_file_lock);
            if (ems_reserve(event_id, num_coords, xs, ys)) {
                fprintf(stderr, "Failed to reserve seats\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_SHOW:
            if (parse_show(valores->file, &event_id) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                break;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            if (ems_show(valores->file_out, event_id)) {
                fprintf(stderr, "Failed to show event\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_LIST_EVENTS:
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            if (ems_list_events(valores->file_out)) {
                fprintf(stderr, "Failed to list events\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_WAIT:
            if (parse_wait(valores->file, &delay, &thread_id) == -1){  // thread_id is not implemented
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                return NULL;
            }
            
            //pthread_mutex_unlock(&write_file_lock);
            pthread_mutex_unlock(&read_file_lock);
            if(thread_id > 0 && delay > 0){
                wait_list[thread_id].size++;
                wait_list[thread_id].waiting_time = realloc(wait_list[thread_id].waiting_time, 
                    ((size_t)wait_list[thread_id].size)*sizeof(unsigned int));
                wait_list[thread_id].waiting_time[wait_list[thread_id].size - 1] = delay;
                break;
            }
            if (delay > 0) {
                for(int t_id = 1; t_id <= max_Threads; t_id++){
                    wait_list[t_id].size++;
                    wait_list[t_id].waiting_time = realloc(wait_list[t_id].waiting_time, 
                        ((size_t)wait_list[t_id].size)*sizeof(unsigned int));
                    wait_list[t_id].waiting_time[wait_list[t_id].size - 1] = delay;//exceto a thread onde estou que vai 
                }
                /*Falta fazer esta parte*/
                //long int escreve = write(valores->file_out,"Waiting...\n",11);
                // if (escreve < 0) {
                //     fprintf(stderr, "Error writing in file\n");
                //     exit(EXIT_FAILURE);
                // }
                //ems_wait(delay);
            }
            break;
        case CMD_INVALID:
            pthread_mutex_unlock(&read_file_lock);
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_HELP:
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            long int escreve = write(valores->file_out, "Available commands:\n"
                "  CREATE <event_id> <num_rows> <num_columns>\n"
                "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                "  SHOW <event_id>\n"
                "  LIST\n"
                "  WAIT <delay_ms> [thread_id]\n"
                "  BARRIER\n"
                "  HELP\n" ,188);
            if (escreve < 0) {
              fprintf(stderr, "Error writing in file\n");
              exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&write_file_lock);
            //pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_BARRIER:
            pthread_mutex_unlock(&read_file_lock);
            barrier = 1;  // Not implemented
            return_value = &barrier;
            //pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_EMPTY:
            pthread_mutex_unlock(&read_file_lock);
            //pthread_mutex_unlock(&write_file_lock);
          break;
        case EOC:
            if (comando == EOC){
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                return (void*) return_value;
            } 
            comando = EOC;
            ems_terminate();
            pthread_mutex_unlock(&read_file_lock);
            //pthread_mutex_unlock(&write_file_lock);
            return (void*) return_value;
        }
    }
    return (void*) return_value;
}

void read_files(char* path, char* name, int max_threads){
    pthread_t thread_id[max_threads + 1];
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    data valores[max_threads + 1];
    max_Threads =max_threads;
    wait_list = (waiting_list*)malloc((size_t)(max_threads + 1) * sizeof(waiting_list));
    int value = 2;
    int *res = &value;
    while (*res != 0){
        for (int i = 1; i <= max_threads; i++){
            valores[i].file = file;
            valores[i].file_out = file_out;
            valores[i].thread_id = i;
            wait_list[i].thread_id = i;
            wait_list[i].size = 0;
            wait_list[i].waiting_time = NULL;
            if (pthread_create(&thread_id[i], NULL, &thread, &valores[i]) != 0){
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
            }
        }
        for (int k = 1; k <= max_threads; k++){
            if (pthread_join(thread_id[k], (void**) &res) != 0){
                fprintf(stderr, "Failed to join thread\n");
                exit(EXIT_FAILURE);
            }
            //if(sizeof(wait_list[k].waiting_time) > 0)
            free(wait_list[k].waiting_time);
        }
        barrier = 0;
    }
    free(wait_list);
    if (close(file) == 1){
        write(STDERR_FILENO, "Error closing file\n", 20);
        exit(EXIT_FAILURE);
    }
    if (close(file_out) == 1){
        write(STDERR_FILENO, "Error closing file\n", 20);
        exit(EXIT_FAILURE);
    }
}