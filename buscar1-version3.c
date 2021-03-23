#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUno (void *);

typedef struct {
  int encontrado;
  int secuencia;
  int * fracciones;
  int * pointerC;
} Vector;

int main(){
    Vector cerosUnos;
    cerosUnos.encontrado = 0;
    cerosUnos.secuencia = 0;

    cerosUnos.pointerC = (int*)calloc(20000, sizeof(int));
    cerosUnos.pointerC[10000] = 1;

    int cantHilos = 0;
    printf("Ingrese la cantidad de hilos que desee crear: ");
    scanf("%d", &cantHilos);

    int fracciones[cantHilos + 1];
    for( int i = 0; i < cantHilos + 1; i++ ) {
        fracciones[i] = (20000 * i ) / cantHilos;
    }
    cerosUnos.fracciones = fracciones;

    pthread_t hilos[cantHilos];
    for( int i = 0; i < cantHilos; i++ )
        pthread_create(&hilos[i], NULL, buscarUno, &cerosUnos);
    
    sleep(1);
    // while(cerosUnos.encontrado == 0) {

    // }

    return 0;
}

void *buscarUno(void *tmp){
    Vector* p = (Vector *)(tmp);
    p->secuencia++;
    // int hilo = p->secuencia;
    for (int i = p->fracciones[p->secuencia - 1]; i < p->fracciones[p->secuencia]; i++)
    {
        if( p->encontrado != 0 )
            pthread_exit(NULL);

        if(p->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n", i);
            // printf("Secuencia: %d \n", hilo);
            p->encontrado = 1;
            pthread_exit(NULL);
        }
    }
}