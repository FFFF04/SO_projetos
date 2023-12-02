#include  "files.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

int open_file_read(char* path, char* file_name){
    char *filename = (char *) malloc(strlen(path) + strlen("/") + strlen(file_name) + 1); //erro para alocar memoria
    strcpy(filename,path);
    strcat(filename,"/");
    strcat(filename,file_name);
    int file = open(filename,O_RDONLY);
    if (file == -1) {
      write(STDERR_FILENO, "Error opening file\n", 19);
      exit(EXIT_FAILURE);
    }
    free(filename);
    return file;
}

int open_file_out(char* path, char* file_name){
    // criar um cadeia de caracteres onde o .jobs e trocado por .out para escrever o output do ficheiro lido
    size_t replace_str_len = strlen("jobs");
    size_t replace_with_len = strlen("out");
    size_t modified_len = strlen(file_name) - replace_str_len + replace_with_len;
    char *file_out_name = (char *)malloc((modified_len + 1) * sizeof(char));
    char *found_position = strstr(file_name, "jobs");
    size_t copy_length = (size_t)(found_position - file_name);
    strncpy(file_out_name, file_name, copy_length);
    file_out_name[copy_length] = '\0';
    strcat(file_out_name, "out");
    char *filename_out = (char *) malloc(strlen(path) + strlen("/") + strlen(file_out_name) + 1);
    strcpy(filename_out,path);
    strcat(filename_out,"/");
    strcat(filename_out,file_out_name);

    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int file_out = open(filename_out, O_CREAT | O_TRUNC | O_WRONLY , filePerms);
    if (file_out == -1) {
      write(STDERR_FILENO, "Error opening file\n", 19);
      exit(EXIT_FAILURE);
    }

    free(file_out_name);
    free(filename_out);

    return file_out;
}