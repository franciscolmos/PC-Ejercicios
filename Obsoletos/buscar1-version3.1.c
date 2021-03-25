#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUno (void *);

typedef struct {
  int encontrado;
  int * pointerC;
} Vector;

typedef struct {
    int inicio;
    int fin;
}Posicion;

typedef struct {
    Vector vector;
    Posicion posicion;
}Hilo;

int main(){
    Vector cerosUnos;
    cerosUnos.encontrado = 0;

    cerosUnos.pointerC = (int*)calloc(20000, sizeof(int));
    cerosUnos.pointerC[10000] = 1; //colocamos un 1 en la posicion 10mil

    int cantHilos = 0;
    printf("Ingrese la cantidad de hilos que desee crear: ");
    scanf("%d", &cantHilos);

    Hilo hilos[cantHilos];
    Posicion rango;

    for( int i = 0; i < cantHilos; i++ ) {
        hilos[i].vector = cerosUnos;
        rango.inicio = (20000 * i) / cantHilos;
        rango.fin = (20000 * (i+1)) / cantHilos;
        hilos[i].posicion.inicio =  rango.inicio;
        hilos[i].posicion.fin = rango.fin;
        printf("\n hilo %d:", i+1);
        printf("inicio: %d, ", hilos[i].posicion.inicio);
        printf("fin: %d", hilos[i].posicion.fin);
    }


    pthread_t hiloss[cantHilos]; // creamos n hilos
    
    for( int i = 0; i < cantHilos; i++ )
        pthread_create(&hiloss[i], NULL, buscarUno, &hilos[i]);
    
    sleep(1);
    // while(cerosUnos.encontrado == 0) {

    // }
    
    return 0;
}

void *buscarUno(void *tmp){
    Hilo* h = (Hilo *)(tmp);
    for (int i = h->posicion.inicio ; i < h->posicion.fin; i++)
    {
        if( h->vector.encontrado != 0 )
            pthread_exit(NULL);

        if(h->vector.pointerC[i] == 1){
            printf("\nSe encontro un 1 en la posicion: %d \n", i);
            h->vector.encontrado = 1;
            pthread_exit(NULL);
        }
    }
}