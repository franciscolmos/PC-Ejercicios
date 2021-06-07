#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <mqueue.h>

int main () {
  int error=0, status=0;

    status = sem_unlink("/semTelefono");
    if (!status)
      printf("Semaforo [semTelefono] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }

    status = sem_unlink("/semLlamadas");
    if (!status)
      printf("Semaforo [semLlamadas] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }

    status = mq_unlink("/encargadoCocineros");
    if (!status) {
      printf("cola [encargadoCocineros] borrado!\n");
    }else{
      perror("mq_close 2");
      error -= 1;
    }
  
  return error;
}

