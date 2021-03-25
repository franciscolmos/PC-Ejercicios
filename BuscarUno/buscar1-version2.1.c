#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUnoPrimeraMitad (void *);
void *buscarUnoSegundaMitad (void *);

typedef struct {
  int encontrado;
  int tamano;
  int randomPosUno;
  int mitad;
  int * pointerC;
} Vector;

int main(){

//  CREACION DEL OBJETO VECTOR E INGRESO POR PANTALLA DEL TAMANO DEL ARREGLO, SE INICIALIZA EN 0 ENCONTRADO
    Vector vector;
    vector.encontrado = 0;
    printf("Ingrese tamano del arreglo: ");
    scanf("%d", &vector.tamano);

//  SE RESERVA TAMANO POSICIONES EN MEMORIA INICIALIZADAS EN 0 CON CALLOC Y SE SETEA LA MITAD DEL ARREGLO
    vector.pointerC = (int*)calloc(vector.tamano, sizeof(int));
    vector.mitad = vector.tamano / 2;

//  SE GENERA UNA POSICION RANDOM PARA EL UNO
    srand(time(NULL));
    vector.randomPosUno = rand() % (vector.tamano);
    vector.pointerC[vector.randomPosUno] = 1;

//  creamos 2 hilos
    pthread_t hilos[2];

//  CADA HILO SE LE ASIGNA LA TAREA DE BUSCAR EN SU MITAD CORRESPONDIENTE Y SE ESPERA A QUE CADA UNO TERMINE
    pthread_create(&hilos[0], NULL, buscarUnoPrimeraMitad, &vector);
    pthread_create(&hilos[1], NULL, buscarUnoSegundaMitad, &vector);
    pthread_join(hilos[0], NULL);
    pthread_join(hilos[1], NULL);

    return 0;
}

//  METODO QUE BUSCA EN LA PRIMERA MITAD, SI ENCUENTRA EL 1 SETEA ENCONTRADO EN 1 (TRUE)
void *buscarUnoPrimeraMitad(void *tmp){
    Vector* vector = (Vector *)(tmp);    
    for (int i = 0; i < vector->mitad; i++)
    {
        if( vector->encontrado != 0 )
            pthread_exit(NULL);
        if(vector->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            vector->encontrado = 1;
            pthread_exit(NULL);
        }
    }
}

//  METODO QUE BUSCA EN LA SEGUNDA MITAD, SI ENCUENTRA EL 1 SETEA ENCONTRADO EN 1 (TRUE)
void *buscarUnoSegundaMitad(void *tmp){
    Vector* vector = (Vector *)(tmp);
    for (int i = vector->mitad; i < 20000; i++)
    {
        if( vector->encontrado != 0 )
            pthread_exit(NULL);
        if(vector->pointerC[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            vector->encontrado = 1;
            pthread_exit(NULL); 
        }
    }
}