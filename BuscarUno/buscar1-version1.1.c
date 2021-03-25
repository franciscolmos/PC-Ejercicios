#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> // se agrega para el uso de time(NULL) para el srand.
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUno (void *);

//  CREAMOS UNA ESTRUCTURA VECTOR CON TODA LOS ATRIBUTOS NECESARIOS PARA OPERAR CON ELLA
typedef struct
{
    int tamano;
    int randomPosUno;
    int *pointerC;
}Vector;

int main(){

//  CREACION DEL OBJETO VECTOR E INGRESO POR PANTALLA DEL TAMANO DEL ARREGLO
    Vector vector;
    vector.tamano = 0;

    printf("Ingrese tamano del arreglo: ");
    scanf("%d", &vector.tamano);

//  SE RESERVA TAMANO ESPACIOS EN MEMRORIA PARA POINTERC Y SE LA INICIALIZA EN 0
    vector.pointerC = (int*)calloc(vector.tamano, sizeof(int));

//  GENERAMOS UNA POSICION RANDOM DONDE INGRESAMOS EL 1
    srand(time(NULL));
    vector.randomPosUno = rand() % (vector.tamano);
    vector.pointerC[vector.randomPosUno] = 1;

//  CREAMOS 1 HILO Y ESPERAMOS A QUE TERMINE CON JOIN, EL HILO EJECTUTA LA FUNCION BUSCARUNO
//  Y SE LE PASA POR PARMAETRO EL VECTOR CON TODA LA INFORMACION PARA PODER ENCONTRAR EL 1
    pthread_t hilos[1];
    pthread_create(&hilos[0], NULL, buscarUno, &vector);
    pthread_join(hilos[0], NULL);

    return 0;
}

//  FUNCION QUE RECIBE *VOID Y LUEGO LO CASTEA A UN VECTOR PARA PODER HACER LA BUSQUEDA SECUENCIAL DEL 1
void *buscarUno(void *tmp){
    Vector* vector = (Vector *)(tmp);
    for (int i = 0; i < vector->tamano; i++)
    {
        if(vector->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            pthread_exit(NULL); 
        }
    }
}