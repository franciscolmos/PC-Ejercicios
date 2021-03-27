#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarPresa (void *);

//  CREAMOS UNA ESTRUCTURA VECTOR CON TODA LOS ATRIBUTOS NECESARIOS PARA OPERAR CON ELLA
typedef struct{
    int tamano;
    int randomPosUno;
    int *arreglo;
}Vector;

typedef struct{
    int inicio;
    int fin;
}Rango;

typedef struct{
    Vector *vector;
    Rango rango;
    int id;
}ZonaBusqueda;

int main(){

//  CREACION DEL OBJETO VECTOR E INGRESO POR PANTALLA DEL TAMANO DEL ARREGLO
    Vector vector;
    vector.tamano = 0;

    while (vector.tamano < 1 || vector.tamano > 50000) {
        printf("Ingrese tamano del arreglo (debe ser un valor entre 1 y 50000): ");
        scanf("%d", &vector.tamano);
    }
    

//  SE RESERVA TAMANO ESPACIOS EN MEMRORIA PARA 'ARREGLO' Y SE LA INICIALIZA EN 0
    vector.arreglo = (int*)calloc(vector.tamano, sizeof(int));

//  GENERAMOS UNA POSICION RANDOM DONDE INGRESAMOS EL 1
    srand(time(NULL));
    vector.randomPosUno = rand() % (vector.tamano);
    vector.arreglo[vector.randomPosUno] = 1;
    printf("La funcion random asigno la presa en la posicion %d\n", vector.randomPosUno);

//  SE INGRESA POR PANTALLA LA CANTIDAD DE HILOS, DEBE SER UN NUMERO ENTRE 2 Y 9 SINO SE PIDE NUEVAMENTE
    int cantHilos = 0;
    while(cantHilos < 2 || cantHilos > 9){
        printf("Ingrese la cantidad de hilos que desee crear (debe ser entre 2 y 9) : ");
        scanf("%d", &cantHilos);
    }
    
//  CREAMOS DOS ARRAY DE TIPO ZONABUSQUEDA Y PTHREAD(HILOS) CON CAPACIDAD = cantHilos
    ZonaBusqueda zonaBusqueda[cantHilos];
    pthread_t hilos[cantHilos];

//  CREAMOS cantHilos INSTANCIAS DEL TIPO ZONABUSQUEDA, ASIGNAMOS EL MISMO VECTOR DE BUSQUEDA PARA TODAS.
//  DEFINIMOS EL RANGO DE CADA UNA Y CREAMOS LOS HILOS
    for( int i = 0; i < cantHilos; i++ ) {
        zonaBusqueda[i].vector = &vector;
        zonaBusqueda[i].rango.inicio = (vector.tamano * i) / cantHilos;
        zonaBusqueda[i].rango.fin = (vector.tamano * (i+1)) / cantHilos;
        zonaBusqueda[i].id = i+1;
        pthread_create(&hilos[i], NULL, buscarPresa, &zonaBusqueda[i]);
        printf("\n hilo %d:\n", i+1);
        printf("\tinicio: %d, ", zonaBusqueda[i].rango.inicio);
        printf("fin: %d", zonaBusqueda[i].rango.fin);
    }

//  ESPERAMOS LA FINALIZACION DE TODOS LOS HILOS PARA CONTINUAR CON EL MAIN
    for (int i = 0; i < cantHilos; i++) {
        pthread_join(hilos[i], NULL);
    }

//  LIBERAMOS EL ESPACIO DE MEMORIA QUE ESTA ASIGNADO POR CALLOC Y APUNTAMOS EL PUNTERO A LA NADA
    free(vector.arreglo);
    vector.arreglo = NULL;
    printf("\n");
    return 0;
}

//  METODO QUE BUSCA EN EL RANGO QUE SE LE ASIGNA A CADA HILO

void *buscarPresa(void *tmp){
    ZonaBusqueda* zonaBusqueda = (ZonaBusqueda *)(tmp);
    for (int i = zonaBusqueda->rango.inicio; i < zonaBusqueda->rango.fin; i++)
    {
        if(zonaBusqueda->vector->arreglo[i] == 1){
            printf("\nEl hilo %d encontro un 1 en la posicion: %d \n ", zonaBusqueda->id, i);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}