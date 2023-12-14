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
int max_Threads, active_threads;
waiting_list *wait_list;
locks *writing_locks = NULL;
int size_writing_locks = 0;
pthread_mutex_t read_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_file_lock = PTHREAD_MUTEX_INITIALIZER;

void del(int n, unsigned int *arr){
    for (int i = 1; i < n; i++)
        arr[i - 1] = arr[i];
    arr = realloc(arr, (size_t)(n - 1) * sizeof(int));
}

int find_event_id(unsigned int event_id){
    for (int i = 0; i < size_writing_locks; i++)
    {
        if (writing_locks[i].event_id == (int)event_id)
            return i;
    }
    return -1;
}

void read_lock(){
    if (pthread_mutex_lock(&read_file_lock) != 0){
        fprintf(stderr, "Error in pthread_mutex_lock()\n");
        exit(EXIT_FAILURE);
    }
}

void read_unlock(){
    if (pthread_mutex_unlock(&read_file_lock)!=0){
        fprintf(stderr, "Error in pthread_mutex_unlock()\n");
        exit(EXIT_FAILURE);
    }
}
void write_lock(){
    if (pthread_mutex_lock(&write_file_lock) != 0){
        fprintf(stderr, "Error in pthread_mutex_lock()\n");
        exit(EXIT_FAILURE);
    }
}

void write_unlock(){
    if (pthread_mutex_unlock(&write_file_lock)!=0){
        fprintf(stderr, "Error in pthread_mutex_unlock()\n");
        exit(EXIT_FAILURE);
    }
}

void writing_lock(unsigned int event_id){
    int index = find_event_id(event_id);
    if (index > 0){
        if (pthread_mutex_lock(&writing_locks[index].write_file_lock) != 0){
            fprintf(stderr, "Error in pthread_mutex_lock()\n");
            exit(EXIT_FAILURE);
        }
    }
}

void writing_unlock(unsigned int event_id){
    int index = find_event_id(event_id);
    if (index > 0){
        if (pthread_mutex_unlock(&writing_locks[index].write_file_lock)!=0){
            fprintf(stderr, "Error in pthread_mutex_unlock()\n");
            exit(EXIT_FAILURE);
        }
    }
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
        read_lock();
        if (wait_list[valores->thread_id].size != 0){
            read_unlock();
            ems_wait(wait_list[valores->thread_id].waiting_time[0]);
            del(wait_list[valores->thread_id].size, wait_list[valores->thread_id].waiting_time);
            wait_list[valores->thread_id].size--;
        }
        
        if (barrier == 1){
            return_value = &barrier;
            read_unlock();
            break;
        }
        int command = get_next(valores->file);
        if(command != EOC){
            active_threads++;
        }
        switch (command) {
        case CMD_CREATE:
            if (parse_create(valores->file, &event_id, &num_rows, &num_columns) != 0){
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                read_unlock();
                break;
            }
            writing_locks = (locks*) realloc(writing_locks,
            ((size_t)(size_writing_locks + 1)) *sizeof(locks));
            if(pthread_mutex_init(&writing_locks[size_writing_locks].write_file_lock,NULL) != 0){
                fprintf(stderr, "Failed to initialise lock\n");
                exit(EXIT_FAILURE);
            }
            writing_locks[size_writing_locks].event_id = (int)(event_id);
            size_writing_locks++;
            
            read_unlock();
            writing_lock(event_id);
            if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
            }
            writing_unlock(event_id);
            break;
        case CMD_RESERVE:
            num_coords = parse_reserve(valores->file, MAX_RESERVATION_SIZE, &event_id,xs,ys);
            if (num_coords == 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                read_unlock();
                break;
            }
            read_unlock();
            writing_lock(event_id);
            if (ems_reserve(event_id, num_coords, xs, ys)) {
                fprintf(stderr, "Failed to reserve seats\n");
            }
            writing_unlock(event_id);
            break;
        case CMD_SHOW:
            if (parse_show(valores->file, &event_id) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                read_unlock();
                break;
            } 
            read_unlock();
            write_lock();
            writing_lock(event_id);
            if (ems_show(valores->file_out, event_id)) {
                fprintf(stderr, "Failed to show event\n");
            }
            writing_unlock(event_id);
            write_unlock();
            break;
        case CMD_LIST_EVENTS:
            read_unlock();

            write_lock();
            writing_lock(event_id);
            if (ems_list_events(valores->file_out)) {
                fprintf(stderr, "Failed to list events\n");
            }
            writing_unlock(event_id);
            write_unlock();

            break;
        case CMD_WAIT:
            if (parse_wait(valores->file, &delay, &thread_id) == -1){
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                read_unlock();
                break;
            }
            read_unlock();
            if ((unsigned int)(max_Threads) < thread_id){
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                break;
            }
            if(thread_id > 0 && delay > 0){
                wait_list[thread_id].size++;
                wait_list[thread_id].waiting_time = realloc(wait_list[thread_id].waiting_time, 
                    ((size_t) wait_list[thread_id].size) * sizeof(unsigned int));
                wait_list[thread_id].waiting_time[wait_list[thread_id].size - 1] = delay;
            }
            else if (delay > 0) {
                for(int t_id = 1; t_id <= max_Threads; t_id++){
                    if((size_t) wait_list[t_id].size > 0 &&
                        (sizeof(wait_list[t_id].waiting_time)!=wait_list[t_id].size)&&
                        (wait_list[t_id].size)){                   
                        wait_list[t_id].waiting_time = realloc(wait_list[t_id].waiting_time, 
                            ((size_t) wait_list[t_id].size) * sizeof(unsigned int));
                        wait_list[t_id].waiting_time[wait_list[t_id].size - 1] = delay;
                    }
                }
            }
            fprintf(stdout, "Waiting...\n");
            break;
        case CMD_INVALID:
            read_unlock();
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            break;
        case CMD_HELP:
            read_unlock();
            fprintf(stdout, "Available commands:\n"
                "  CREATE <event_id> <num_rows> <num_columns>\n"
                "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                "  SHOW <event_id>\n"
                "  LIST\n"
                "  WAIT <delay_ms> [thread_id]\n"
                "  BARRIER\n"
                "  HELP\n");
            break;
        case CMD_BARRIER:
            read_unlock();
            barrier = 1;
            return_value = &barrier;
            break;
        case CMD_EMPTY:
            read_unlock();
          break;
        case EOC:
            if (comando == EOC){
                read_unlock();
                return (void*) return_value;
            }
            comando = EOC;
            read_unlock();
            
            while(active_threads > 0){}
            ems_terminate();
            return (void*) return_value;
        }
        active_threads--;
    }
    return (void*) return_value;
}

void read_files(char* path, char* name, int max_threads){
    pthread_t thread_id[max_threads + 1];
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    data valores[max_threads + 1];
    active_threads = 0;
    max_Threads = max_threads;
    wait_list = (waiting_list*) malloc((size_t)(max_threads + 1) * sizeof(waiting_list));
    writing_locks = NULL;
    int value = 2;
    int *res = &value;
    while (*res != 0){
        barrier = 0;
        active_threads = 0;
        for (int i = 1; i <= max_threads; i++){
            valores[i].file = file;
            valores[i].file_out = file_out;
            valores[i].thread_id = i;
            wait_list[i].thread_id = i;
            wait_list[i].size = 0;
            wait_list[i].waiting_time = NULL;
        }
        for (int i = 1; i <= max_threads; i++){
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
        }
    }
    fflush(stdout);
    free(writing_locks);
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