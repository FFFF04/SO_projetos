#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }
  char *pipe_name = "";
  char* endptr;
  int fcli, fserv;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  //TODO: Intialize server, create worker threads
  /*
  COM ISTO JA TEMOS O SERVIDOR INICIALIZADO FALTA AS WORKER THREADS
  PARA ISSO ACHO QUE TEMOS DE:
    QUANDO QUERIAMOS UMA NOVO CLIENTE CRIASSE TAMBEM UMA THREAD NOVA QUE 
    FICA ASSOCIADA A ESSE CLINTE
  */
  pipe_name = argv[1];
  unlink(pipe_name);
  if (unlink(pipe_name) != 0 && errno != ENOENT) {
    fprintf(stderr, "unlink failed\n");
    exit(EXIT_FAILURE);
  }
  if (mkfifo(pipe_name, 0777) < 0){
    fprintf(stderr, "mkfifo failed\n");
    exit(EXIT_FAILURE);
  } 

  fserv = open(pipe_name, O_RDONLY);
  if (fserv == -1) {
    fprintf(stderr, "Server open failed\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    /*OK JA PERCEBI NO API NOS CRIAMOS O PIPE DO CLINTE
    DENTRO DA FUNÇAO DO EMS_SETUP VAI ESCREVER NO PIPE_NAME 
    DESTE LADO LEMOS MEIO QUE INSCREVEMOS O DUDE NO SERVER */
    
    //TODO: Read from pipe

    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server
  close(fserv);
  unlink(pipe_name);
  ems_terminate();
}