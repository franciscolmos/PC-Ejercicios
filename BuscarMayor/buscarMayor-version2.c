#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarMayorPrimerMitad (void *);
void *buscarMayorSegundaMitad (void *);

typedef struct 
{
    int mayor;
    int posMayor;
    int *vector;
    int nroHilo;
}Hilo;

int main(){
    int *pointerC;

    pointerC = (int*)calloc(20000, sizeof(int));

    srand(time(NULL));

    for( int i = 0; i < 20000; i++ ) {
        pointerC[i] = rand() % 100000;
    }

    Hilo hilo;
    hilo.vector = pointerC;
    hilo.mayor = 0;
    hilo.posMayor = -1;
    hilo.nroHilo = -1;

    pthread_t hilos[2]; // creamos 1 hilo

    pthread_create(&hilos[0], NULL, buscarMayorPrimerMitad, &hilo);
    pthread_create(&hilos[1], NULL, buscarMayorSegundaMitad, &hilo);
    pthread_join(hilos[0], NULL);
    pthread_join(hilos[1], NULL);

    printf("El mayor es %d, fue encontrado por el hilo %d en la posicion %d.\n", hilo.mayor, hilo.nroHilo, hilo.posMayor);

    return 0;
}

void *buscarMayorPrimerMitad(void *tmp){
    Hilo* hilo = (Hilo *)(tmp);
    for (int i = 0; i < 10000; i++)
    {
        if(hilo->vector[i] > hilo->mayor){
            hilo->mayor = hilo->vector[i];
            hilo->posMayor = i;
            hilo->nroHilo = 1;
        }
    }
    pthread_exit(NULL); 
}

void *buscarMayorSegundaMitad(void *tmp){
    Hilo* hilo = (Hilo *)(tmp);
    for (int i = 10000; i < 20000; i++)
    {
        if(hilo->vector[i] > hilo->mayor){
            hilo->mayor = hilo->vector[i];
            hilo->posMayor = i;
            hilo->nroHilo = 2;
        }
    }
    pthread_exit(NULL); 
}