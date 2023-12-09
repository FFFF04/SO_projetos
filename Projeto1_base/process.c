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
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t read_file_lock = PTHREAD_RWLOCK_INITIALIZER;

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
    while (comando != EOC){
        pthread_mutex_lock(&file_lock);
        if (barrier == 1){
            pthread_mutex_unlock(&file_lock);
            break;
        } 
        number_threads++;
        if(confirma_wait()){
            printf("Waiting...\n");
            ems_wait(delay);
        }
        valores->command = get_next(valores->file);
        //printf("comando : %d\n", comando);
        //printf("Create : %d\n", valores->command);
        //barrier_wait(0);
        active_threads++;
        
        switch (valores->command) {
        case CMD_CREATE:
            //barrier = 1;
            if (parse_create(valores->file, &event_id, &num_rows, &num_columns) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                //free(arg);
                pthread_mutex_unlock(&file_lock);
                break;
                //return NULL;
            }
            //pthread_mutex_unlock(&lock);
            if (ems_create(event_id, num_rows, num_columns)) {
                fprintf(stderr, "Failed to create event\n");
            }
            
            pthread_mutex_unlock(&file_lock);
            break;
            //return NULL;
        //pthread_mutex_lock(&lock);
        case CMD_RESERVE:
            num_coords = parse_reserve(valores->file, MAX_RESERVATION_SIZE, &event_id,xs,ys);
            if (num_coords == 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&file_lock);
                break;
                //return NULL;
            }
            //pthread_mutex_unlock(&lock);
            //pthread_mutex_unlock(&file_lock);
            if (ems_reserve(event_id, num_coords, xs,ys)) {
                fprintf(stderr, "Failed to reserve seats\n");
            }
            pthread_mutex_unlock(&file_lock);
            break;
        case CMD_SHOW:
            if (parse_show(valores->file, &event_id) != 0) {
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&file_lock);
                break;
                //return NULL;
            }
            if (ems_show(valores->file_out, event_id)) {
                fprintf(stderr, "Failed to show event\n");
            }
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&file_lock);
            break;
        /*ISTO PROVAVELMENTE TAMBEM VAI PARA DENTRO DO FICHEIRO*/
        case CMD_LIST_EVENTS:
            if (ems_list_events(valores->file_out)) {
                fprintf(stderr, "Failed to list events\n");
            }
            pthread_mutex_unlock(&file_lock);
            break;
        case CMD_WAIT:
            //pid_t thread_id = gettid();
            if (parse_wait(valores->file, &delay, /*NULL*/&thread_id) == -1) {  // thread_id is not implemented
                fprintf(stderr, "Invalid command. See HELP for usage\n");
                pthread_mutex_unlock(&file_lock);
                return NULL;
            }
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&file_lock);
            if(thread_id > 0 && delay > 0){
                waiting_list[n_waiting] = delay;
                thread_ids[n_waiting++] = thread_id;
                break;
            }
            /*ISTO PROVAVELMENTE TAMBEM VAI PARA DENTRO DO FICHEIRO*/
            if (delay > 0) {
                printf("Waiting...\n");
                ems_wait(delay);
            }
            /*if(thread_id !=NULL){}*/
            break;
        case CMD_INVALID:
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            pthread_mutex_unlock(&file_lock);
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
            pthread_mutex_unlock(&file_lock);
            break;
        case CMD_BARRIER:
            barrier = 1;  // Not implemented
            pthread_mutex_unlock(&file_lock);
            break;
        case CMD_EMPTY:
            pthread_mutex_unlock(&file_lock);
          break;
        case EOC:
            //pthread_mutex_lock(&lock);
            if (comando == EOC){
                pthread_mutex_unlock(&file_lock);
                return NULL;
            } 
            comando = EOC;
            /*barrier = 1;
            barrier_wait(1);*/
            ems_terminate();
            //pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&file_lock);
            return NULL;
        }
        active_threads--;
        //pthread_mutex_unlock(&lock);
        
    }
    
    return NULL;
}

void read_files(char* path, char* name, int max_threads){
    pthread_t thread_id[max_threads];
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    data valores[max_threads]; 
    /*if (pthread_mutex_init(&lock, NULL) != 0) {
       fprintf(stderr, "Failed to create lock\n");
        exit(EXIT_FAILURE);
    }*/
    while (comando != EOC){
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
            if (pthread_join(thread_id[k],NULL) != 0){
                fprintf(stderr, "Failed to join thread\n");
                exit(EXIT_FAILURE);
            }
        }
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
    //printf("paragem:%d\n",paragem);
    //fflush(stdout);
}
    /*if (pthread_create(&thread_id, NULL, &thread_mom, &valor) != 0){
        fprintf(stderr, "Failed to create thread\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(thread_id, NULL) != 0){
        fprintf(stderr, "Failed to join1 thread\n");
        exit(EXIT_FAILURE);
    }
    return;*/
    //pthread_t thread_id;
    /*
    pthead mae le cm 
    max_pthe 2 
    for ate 2  //while
        le essas duas linhas
        lanca as ptheads filhas
        //barrier-
             //while (n_threads>0)
                    //join
    */
    /*while (1){
        if (pthread_create(&thread_id,NULL,&thread_mom,NULL) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
        if (pthread_join(thread_id,NULL) != 0){
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
        }
        if (comando == EOC) break;
    }*/
    
    /*
    int file = open_file_read(path, name);
    int file_out = open_file_out(path, name);
    pthread_t thread_id[max_threads];
    data valores[max_threads];
    int j = 0;
    for (int i = 0; i < max_threads; i++){ 
            j++;
            valores[i].file = file;
            valores[i].file_out = file_out;
            if (pthread_create(&thread_id[i],NULL,&thread,&valores[i]) != 0){
                fprintf(stderr, "Failed to create thread\n");
                exit(EXIT_FAILURE);
            }
            if (comando == EOC) break;
        }
        for (int k = 0; k < j; k++){
            if (pthread_join(thread_id[k],NULL) != 0){
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
            }
        }
        j = 0;
        //printf("paragem:%d\n",paragem);
        //fflush(stdout);
        if (comando == EOC) break;
    
    */

/*void* thread_mom(void* arg){
    data *values = (data*) arg;
    int number_threads = 0;
    int i = 0;
    int k = 0;
    data valores[values->max_threads];
    pthread_t thread_id[values->max_threads];////alterar indice que controla
    while (1){
      //printf("%d\n",comando);
      //pthread_t thread_id[i];
      //void* return_value;
        //printf("%d\n",comando);
        if(number_threads >= values->max_threads){ //(i%max_threads)
            k = (k + 1)%values->max_threads;//indice que controla indice da primeira thread criada ainda ativa (//if(i==k)tinha acho desnecessario)
            //printf("indice join%d\n", i);
            if (pthread_join(thread_id[i], NULL) != 0){
                fprintf(stderr, "Failed to join1 thread\n");
                exit(EXIT_FAILURE);
            }
            number_threads--;
        }
        //valores->command = comando;
        valores[i].file = values->file;
        valores[i].file_out = values->file_out;
        //valores[i].command = get_next(values->file);
        if (comando == EOC) break;
        if (pthread_create(&thread_id[i], NULL, &thread, &valores[i]) != 0){
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
        //printf("indice create%d %d\n", i, k);
        //int* comando = (int*)return_value;
        if (comando == EOC) break;
        //FILHO DE PAIS MATEMATICOS
        number_threads++; //max=3 (0,1,2)3
        i = (i + 1)%values->max_threads;
        //free(comando);
        //free(valores);
    }
    while(number_threads > 0){
        //printf("indice join 2_%d\n", k);
        if (pthread_join(thread_id[k], NULL) != 0){
            fprintf(stderr, "Failed to join2 thread\n");
            exit(EXIT_FAILURE);
        }
        k = (k + 1)%values->max_threads;
        number_threads--;
    }
    return NULL;
}*/