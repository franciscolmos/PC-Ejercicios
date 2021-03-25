#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarUno (void *);

//  CREACION DE LAS ESTRUCTURAS NECESARIAS DE FORMA ANIDADA
typedef struct {
  int encontrado;
  int tamano;
  int randomPosUno;
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

//  CREACION DEL VECTOR, SE INICIALIZA ENCONTRADO EN 0 (FALSE), SE INGRESA POR PANTALLA EL TAMANO DEL ARREGLO Y SE RESERVA
//  EN MEMORIA DICHO ESPACIO INICIALIZADOS EN 0 CON CALLOC
    Vector vector;
    vector.encontrado = 0;
    printf("Ingrese tamano del arreglo: ");
    scanf("%d", &vector.tamano);
    vector.pointerC = (int*)calloc(vector.tamano, sizeof(int));

//  SE GENERA UNA POSICION RANDOM PARA INSERTAR EL 1 EN ARREGLO DE CEROS
    srand(time(NULL));
    vector.randomPosUno = rand() % (vector.tamano);
    vector.pointerC[vector.randomPosUno] = 1;

//  SE INGRESA POR PANTALLA LA CANTIDAD DE HILOS, DEBE SER UN NUMERO ENTRE 2 Y 9 SINO SE PIDE NUEVAMENTE
    int cantHilos = 0;
    while(cantHilos < 2 || cantHilos > 9){
        printf("Ingrese la cantidad de hilos que desee crear (debe ser entre 2 y 9) : ");
        scanf("%d", &cantHilos);
    }
    
//  SE CREA UN ARRAY DE HILOS CON LA CANTIDAD ANTES INGRESADA Y SE GENERA EL RANGO PARA CADA HILO DONDE VA A BUSCAR
    Hilo hilos[cantHilos];
    Posicion rango;
    for( int i = 0; i < cantHilos; i++ ) {
        hilos[i].vector = vector;
        rango.inicio = (vector.tamano * i) / cantHilos;
        rango.fin = (vector.tamano * (i+1)) / cantHilos;
        hilos[i].posicion.inicio =  rango.inicio;
        hilos[i].posicion.fin = rango.fin;
        printf("\n hilo %d:\n", i+1);
        printf("\tinicio: %d, ", hilos[i].posicion.inicio);
        printf("fin: %d", hilos[i].posicion.fin);
    }

//  CREAMOS N HILOS Y SE LE ASIGNA A CADA UNO LA TAREA DE BUSCAR EN UN RANGO ESPECIFICO DEL ARREGLO
    pthread_t hiloss[cantHilos];
    for( int i = 0; i < cantHilos; i++ )
        pthread_create(&hiloss[i], NULL, buscarUno, &hilos[i]);
    
    sleep(1);
    // while(cerosUnos.encontrado == 0) {

    // }
    
    return 0;
}

//  METODO ENCARGADO DE BUSCAR EL 1. RECIBE UN VOID* QUE SE CASTEA A HILO Y BUSCA EN EL ARREGLO ENTRE EL RANGO ESPECIFICADO EL 1.
//  SI LO ENCUENTRA PONE EN 1 (TRUE) EL ATRIBUTO ENCONTRADO DE VECTOR
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