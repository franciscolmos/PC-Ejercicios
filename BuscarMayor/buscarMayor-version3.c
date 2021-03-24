#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarMayor (void *);

int mayor = 0;
int posMayor = -1;
int nroHilo = -1;

typedef struct {
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

    srand(time(NULL));

    // INICIALIZAMOS EL VECTOR DE NUMEROS PARA BUSCAR EL MAYOR
    Vector vectorMayor;

    vectorMayor.pointerC = (int*)calloc(20000, sizeof(int));
    for( int i = 0; i < 20000; i++ ) {
        vectorMayor.pointerC[i] = rand() % 100000;
    }

    // EL USUARIO INGRESA LA CANTIDAD DE HILOS Y CREAMOS N ESRTUCTURAS HILOS
    int cantHilos = 0;
    printf("Ingrese la cantidad de hilos que desee crear: ");
    scanf("%d", &cantHilos);
    int delta = 20000 / cantHilos;

    Hilo hilos[cantHilos];
    Posicion rango;

    // SETEAMOS EL RANGO DE BUSQUEDA DE CADA HILO
    for( int i = 0; i < cantHilos; i++ ) {
        hilos[i].vector = vectorMayor;
        rango.inicio = (20000 * i) / cantHilos;
        rango.fin = (20000 * (i+1)) / cantHilos;
        hilos[i].posicion.inicio =  rango.inicio;
        hilos[i].posicion.fin = rango.fin;
        // printf("\n hilo %d:", i+1);
        // printf("inicio: %d, ", hilos[i].posicion.inicio);
        // printf("fin: %d", hilos[i].posicion.fin);
    }


    // CREAMOS N HILOS
    pthread_t hiloss[cantHilos]; 
    for( int i = 0; i < cantHilos; i++ )
        pthread_create(&hiloss[i], NULL, buscarMayor, &hilos[i]);

    sleep(1);

    nroHilo = 1 + (posMayor / delta);

    printf("El mayor es %d, fue encontrado por el hilo %d en la posicion %d\n", mayor, nroHilo, posMayor);
    
    return 0;
}

void *buscarMayor(void *tmp) {
    Hilo* hilo = (Hilo *)(tmp);
    for (int i = hilo->posicion.inicio; i < hilo->posicion.fin; i++)
    {
        if(hilo->vector.pointerC[i] > mayor){
            mayor = hilo->vector.pointerC[i];
            posMayor = i;
        }
    }
    pthread_exit(NULL);
}