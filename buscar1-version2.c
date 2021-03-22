#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUnoPrimeraMitad (void *);
void *buscarUnoSegundaMitad (void *);

typedef struct {
  int encontrado;
  int mitad;
  int * pointerC;
} Vector;

int main(){
    Vector cerosUnos;
    cerosUnos.encontrado = 0;
    cerosUnos.pointerC = (int*)calloc(20000, sizeof(int));
    cerosUnos.mitad = 20000 / 2;

    cerosUnos.pointerC[15000] = 1;

    pthread_t hilos[2]; // creamos 2 hilos

    pthread_create(&hilos[0], NULL, buscarUnoPrimeraMitad, &cerosUnos);
    pthread_create(&hilos[1], NULL, buscarUnoSegundaMitad, &cerosUnos);
    pthread_join(hilos[0], NULL);
    pthread_join(hilos[1], NULL);

    return 0;
}

void *buscarUnoPrimeraMitad(void *tmp){
    Vector* p = (Vector *)(tmp);    
    for (int i = 0; i < p->mitad; i++)
    {
        if( p->encontrado != 0 )
            pthread_exit(NULL);
        if(p->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            p->encontrado = 1;
            pthread_exit(NULL);
        }
    }
}

void *buscarUnoSegundaMitad(void *tmp){
    Vector* p = (Vector *)(tmp);
    for (int i = p->mitad; i < 20000; i++)
    {
        if( p->encontrado != 0 )
            pthread_exit(NULL);
        if(p->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            p->encontrado = 1;
            pthread_exit(NULL); 
        }
    }
}