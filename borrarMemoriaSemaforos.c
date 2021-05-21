#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

int main () {
    int status=0;
    // Semaforo Telefono
    status = sem_unlink("/semTelefono");
    if (status)
        perror("sem_unlink()");
    else
        perror("sem_close()");

    // Semaforo Llamadas
    status = sem_unlink("/semLlamadas");
    if (status)
        perror("sem_unlink()");
    else
        perror("sem_close()");

    // Semaforo PedidosPorCobrar
    status = sem_unlink("/semPedidosPorCobrar");
    if (status)
        perror("sem_unlink()");
    else
        perror("sem_close()");

    // Semaforo DejarDinero
    status = sem_unlink("/semDejarDinero");
    if (status)
        perror("sem_unlink()");
    else
        perror("sem_close()");

    // Semaforo TomarDinero
    status = sem_unlink("/semCobrarDinero");
    if (status)
        perror("sem_unlink()");
    else
        perror("sem_close()");

    // Memoria Compartida
    status = shm_unlink("/memCompartida");
    if (status)
      perror("unlink()");
    else 
      printf("Descriptor de memoria borrado!\n");
}