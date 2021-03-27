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
}ZonaBusqueda;

int main(){

//  CREACION DEL OBJETO VECTOR E INGRESO POR PANTALLA DEL TAMANO DEL ARREGLO
    Vector vector;
    vector.tamano = 0;

    while (vector.tamano < 1 || vector.tamano > 50000)
    {
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

//  CREAMOS ZONA BUSQUEDA PARA PASAR EL VECTOR Y EL RANGO SOBRE EL QUE DEBE BUSCAR EL HILO
    ZonaBusqueda zonaBusqueda;
    zonaBusqueda.vector = &vector;
    zonaBusqueda.rango.inicio = 0;
    zonaBusqueda.rango.fin = vector.tamano;

/*
    CREAMOS 1 HILO Y ESPERAMOS A QUE TERMINE CON JOIN, EL HILO EJECTUTA LA FUNCION BUSCARUNO
    Y SE LE PASA POR PARMAETRO EL VECTOR CON TODA LA INFORMACION PARA PODER ENCONTRAR EL 1
*/
    pthread_t hilo;
    pthread_create(&hilo, NULL, buscarPresa, &zonaBusqueda);
    pthread_join(hilo, NULL);

//  LIBERAMOS EL ESPACIO DE MEMORIA QUE ESTA ASIGNADO POR CALLOC Y APUNTAMOS EL PUNTERO A LA NADA
    free(vector.arreglo);
    vector.arreglo = NULL;
    return 0;
}

//  FUNCION QUE RECIBE *VOID Y LUEGO LO CASTEA A UNA ZONABUSQUEDA PARA PODER HACER LA BUSQUEDA SECUENCIAL DEL 1
void *buscarPresa(void *tmp){
    ZonaBusqueda* zonaBusqueda = (ZonaBusqueda *)(tmp);
    for (int i = 0; i < zonaBusqueda->vector->tamano; i++)
    {
        if(zonaBusqueda->vector->arreglo[i] == 1){
            printf("Se encontro un 1 en la posicion: %d \n ", i);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}