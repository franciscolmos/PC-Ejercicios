#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarMayor (void *);

int main(){
    int *pointerC;

    pointerC = (int*)calloc(20000, sizeof(int));

    srand(time(NULL));

    for( int i = 0; i < 20000; i++ ) {
        pointerC[i] = rand() % 100000;
    }

    pthread_t hilos[1]; // creamos 1 hilo

    pthread_create(&hilos[0], NULL, buscarMayor, pointerC);
    pthread_join(hilos[0], NULL);

    return 0;
}

void *buscarMayor(void *tmp){
    int* p = (int *)(tmp);
    int mayor = 0;
    int posMayor = 0;
    for (int i = 0; i < 20000; i++)
    {
        if(p[i] > mayor){
            mayor = p[i];
            posMayor = i;
        }
    }
    printf("El mayor es %d y la posicion es %d \n", mayor, posMayor);
    
    pthread_exit(NULL); 
}