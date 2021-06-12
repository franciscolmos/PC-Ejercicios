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
      perror("semTelefono_unlink()");
      error -= 1;
    }

    status = sem_unlink("/semCobrarPedidos");
    if (!status)
      printf("Semaforo [semCobrarPedidos] borrado!\n");
    else
      perror("semCobrarPedidos_unlink");

    status = mq_unlink("/encargadoCocineros");
    if (!status) {
      printf("cola [encargadoCocineros] borrado!\n");
    }else{
      perror("mq_encargadoCocineros");
      error -= 1;
    }

    status = mq_unlink("/cocinerosDelivery");
    if (!status) {
      printf("cola [cocinerosDelivery] borrado!\n");
    }else{
      perror("mq_cocinerosDelivery");
    }
    
    status = unlink("/tmp/deliveryEncargado");
    if (!status)
      printf("FIFO [deliveryEncargado] borrada!\n");
    else
      perror("FIFO_unlink");
    
  
  return error;
}

