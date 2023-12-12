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


//pthread_mutex_t lock;
int comando;
int barrier = 0; //0 nao ha barrier 1 ha barrier
int active_threads = 0;
unsigned int number_threads = 0;
int n_waiting = 0;
unsigned int thread_ids[100];
unsigned int waiting_list[100];
pthread_mutex_t read_file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_file_lock = PTHREAD_MUTEX_INITIALIZER;

int confirma_wait(){
    for (int i = 0; i < n_waiting; i++){
        if(number_threads == thread_ids[i])
            return 1;
    }
    return 0;
}

void* thread(void* arg){
    data *valores = (data*) arg;
    unsigned int event_id, delay ,thread_id;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int *return_value = malloc(sizeof(int));
    int value;
    return_value = &value;
    while (comando != EOC){
        pthread_mutex_lock(&read_file_lock);
        if (barrier == 1){
            return_value = &barrier;
            pthread_mutex_unlock(&read_file_lock);
            break;
        } 
        number_threads++;
        // if(confirma_wait()){
        //     printf("Waiting...\n");
        //     ems_wait(delay);
        // }
        valores->command = get_next(valores->file);
        //printf("comando : %d\n", comando);
        //printf("Create : %d\n", valores->command);
        //barrier_wait(0);
        //pthread_mutex_lock(&write_file_lock);
        active_threads++;
        
        switch (valores->command) {
        case CMD_CREATE:
            //barrier = 1;
            if (parse_create(valores->file, &event_id, &num_rows, &num_columns) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                //free(arg);
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                break;
                //return NULL;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            //pthread_mutex_unlock(&lock);
            if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
            //return NULL;
        // pthread_mutex_lock(&lock);
        case CMD_RESERVE:
            num_coords = parse_reserve(valores->file, MAX_RESERVATION_SIZE, &event_id,xs,ys);
            if (num_coords == 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                break;
                //return NULL;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            //pthread_mutex_unlock(&lock);
            //pthread_mutex_unlock(&read_file_lock);
            if (ems_reserve(event_id, num_coords, xs,ys)) {
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
                //return NULL;
            }
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            if (ems_show(valores->file_out, event_id)) {
                fprintf(stderr, "Failed to show event\n");
            }
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&write_file_lock);
            break;
        /*ISTO PROVAVELMENTE TAMBEM VAI PARA DENTRO DO FICHEIRO*/
        case CMD_LIST_EVENTS:
            pthread_mutex_unlock(&read_file_lock);
            pthread_mutex_lock(&write_file_lock);
            if (ems_list_events(valores->file_out)) {
                fprintf(stderr, "Failed to list events\n");
            }
            pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_WAIT:
            //pid_t thread_id = gettid();
            if (parse_wait(valores->file, &delay, /*NULL*/&thread_id) == -1) {  // thread_id is not implemented
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&read_file_lock);
                return NULL;
            }
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&read_file_lock);
            
            //pthread_mutex_unlock(&write_file_lock);
            if(thread_id > 0 && delay > 0){
                waiting_list[n_waiting] = delay;
                thread_ids[n_waiting++] = thread_id;
                break;
            }
            /*ISTO PROVAVELMENTE TAMBEM VAI PARA DENTRO DO FICHEIRO*/
            if (delay > 0) {
                long int escreve = write(valores->file_out,"Waiting...\n",11);
                if (escreve < 0) {
                fprintf(stderr, "Error writing in file\n");
                exit(EXIT_FAILURE);
                }
                ems_wait(delay);
            }
            /*if(thread_id !=NULL){}*/
            break;
        case CMD_INVALID:
            pthread_mutex_unlock(&read_file_lock);
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            //pthread_mutex_unlock(&write_file_lock);
            break;
        case CMD_HELP:
            pthread_mutex_unlock(&read_file_lock);
            //pthread_mutex_lock(&write_file_lock);
            long int escreve = write(valores->file_out, "Available commands:\n"
                "  CREATE <event_id> <num_rows> <num_columns>\n"
                "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
                "  SHOW <event_id>\n"
                "  LIST\n"
                "  WAIT <delay_ms> [thread_id]\n"
                "  BARRIER\n"
                "  HELP\n" ,188);
            //pthread_mutex_unlock(&write_file_lock);
            if (escreve < 0) {
              fprintf(stderr, "Error writing in file\n");
              exit(EXIT_FAILURE);
            }
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
            //pthread_mutex_lock(&lock);
            if (comando == EOC){
                pthread_mutex_unlock(&read_file_lock);
                //pthread_mutex_unlock(&write_file_lock);
                return (void*) return_value;
            } 
            comando = EOC;
            /*barrier = 1;
            barrier_wait(1);*/
            ems_terminate();
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&read_file_lock);
            //pthread_mutex_unlock(&write_file_lock);
            return (void*) return_value;
        }
        active_threads--;
        //pthread_mutex_unlock(&lock);
    }
    return (void*) return_value;
}

void read_files(char* path, char* name, int max_threads){
    pthread_t thread_id[max_threads];
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    data valores[max_threads];
    int value = 2;
    int *res = &value;
    while (res != 0){
        for (int i = 0; i < max_threads; i++){ 
            valores[i].file = file;
            valores[i].file_out = file_out;
            valores[i].max_threads = max_threads;
            if (pthread_create(&thread_id[i],NULL,&thread,&valores[i]) != 0){
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
            }
        }
        for (int k = 0; k < max_threads; k++){
            if (pthread_join(thread_id[k],(void**) &res) != 0){
                fprintf(stderr, "Failed to join thread\n");
                exit(EXIT_FAILURE);
            }
        }
        // printf("%ls",res);
        barrier = 0;
    }
    
    if (close(file) == 1){
        write(STDERR_FILENO, "Error closing file\n", 20);
        exit(EXIT_FAILURE);
    }
    if (close(file_out) == 1){
        write(STDERR_FILENO, "Error closing file\n", 20);
        exit(EXIT_FAILURE);
    }
}