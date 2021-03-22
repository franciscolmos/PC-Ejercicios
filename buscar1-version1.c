#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUno (void *);

int main(){
    int *pointerC;

    pointerC = (int*)calloc(20000, sizeof(int));

    pointerC[10000] = 1;

    //printf("%d \n", pointerC[10000]);

    pthread_t hilos[1]; // creamos 1 hilo

    pthread_create(&hilos[0], NULL, buscarUno, pointerC);
    pthread_join(hilos[0], NULL);

    return 0;
}

void *buscarUno(void *tmp){
    int* p = (int *)(tmp);
    for (int i = 0; i < 20000; i++)
    {
        if(p[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            pthread_exit(NULL); 
        }
    }
}