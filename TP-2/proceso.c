#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

int main(){
    int puntuacion = 0; // Guarda la puntuacion del juego

    // Creamos los pipelines entre tel-enc. Tmb los semaforos

    // Creamos las colas de mensajes para enc-coc y coc-del

    // Creamos una fifo para del-enc. Tmb los semaforos

    // Se crean los actores del juego donde pasamos lo que creamos recien
    // Telefono * telefono = crearTelefono(&puntuacion);
    // Encargado * encargado = crearEncargado(telefono, monitorComandas, memoria);
    // Cocinero * cocinero = crearCocinero(monitorComandas, monitorPedidos);
    // Delivery * delivery = crearDelivery(monitorPedidos, memoria);

    pid_t pid;

    pid = fork();
    if(pid == 0)
        printf("Soy el hijo numero 1. El telefono\n");
    else if(pid > 0) {
        pid = fork();
        if(pid == 0)
            printf("Soy el hijo numero 2. Los cocineros\n");
        else if(pid > 0) {
            pid = fork();
            if(pid == 0)
                printf("Soy el hijo numero 3. Los deliverys\n");
        }
    }

    // Arranca el ciclo de juego del encargado, donde se detecta las teclas que presiona
    if(pid > 0){
        // jugar(encargado);
    }
    
    // Se liberan los semaforos
    // borrarSemMem(encargado, memoria);

    // Se libera la memoria de los objetos creados
    // free(telefono);
    // free(encargado);
    // free(cocinero);
    // free(delivery);

    // return puntuacion;
}