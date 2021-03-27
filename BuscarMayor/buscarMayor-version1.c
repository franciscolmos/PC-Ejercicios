#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

#define P 1000000 // LIMITE MAXIMO DE VALORES ASIGNADOS AL ARREGLO

void *buscarMayor (void *);

//  CREAMOS UNA ESTRUCTURA VECTOR CON TODA LOS ATRIBUTOS NECESARIOS PARA OPERAR CON ELLA
typedef struct{
    int tamano;
    int mayor;
    int posMayor;
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
//  INICIALIZAMOS LOS ATRIBUTOS MAYOR Y POSMAYOR EN -1 PARA VERIFICAR QUE SE ENCUENTRE CORRECTAMENTE
    Vector vector;
    vector.tamano = 0;
    vector.mayor = -1;
    vector.posMayor = -1;

    while (vector.tamano < 1 || vector.tamano > 50000)
    {
        printf("Ingrese tamano del arreglo (debe ser un valor entre 1 y 50000): ");
        scanf("%d", &vector.tamano);
    }
    
//  SE RESERVA TAMANO ESPACIOS EN MEMRORIA PARA 'ARREGLO' Y SE LA INICIALIZA EN 0
    vector.arreglo = (int*)calloc(vector.tamano, sizeof(int));

//  AGREGAMOS ALEATORIAMENTE VALORES AL ARREGLO, ENTRE 0 Y P-1.
    srand(time(NULL));
    for( int i = 0; i < vector.tamano; i++ ) {
        vector.arreglo[i] = rand() % P;
    }

//  CREAMOS ZONA BUSQUEDA PARA PASAR EL VECTOR Y EL RANGO SOBRE EL QUE DEBE BUSCAR EL HILO
    ZonaBusqueda zonaBusqueda;
    zonaBusqueda.vector = &vector;
    zonaBusqueda.rango.inicio = 0;
    zonaBusqueda.rango.fin = vector.tamano;

/*
    CREAMOS 1 HILO Y ESPERAMOS A QUE TERMINE CON JOIN, EL HILO EJECTUTA LA FUNCION BUSCARMAYOR
    Y SE LE PASA POR PARMAETRO EL VECTOR CON TODA LA INFORMACION PARA PODER ENCONTRARLO
*/
    pthread_t hilo;
    pthread_create(&hilo, NULL, buscarMayor, &zonaBusqueda);
    pthread_join(hilo, NULL);

//  LIBERAMOS EL ESPACIO DE MEMORIA QUE ESTA ASIGNADO POR CALLOC Y APUNTAMOS EL PUNTERO A LA NADA
    free(vector.arreglo);
    vector.arreglo = NULL;
    return 0;
}

//  FUNCION QUE RECIBE *VOID Y LUEGO LO CASTEA A UNA ZONABUSQUEDA PARA PODER HACER LA BUSQUEDA SECUENCIAL DEL 1
void *buscarMayor(void *tmp){
    ZonaBusqueda* zonaBusqueda = (ZonaBusqueda *)(tmp);
    for (int i = zonaBusqueda->rango.inicio; i < zonaBusqueda->rango.fin; i++)
    {
        if(zonaBusqueda->vector->arreglo[i] > zonaBusqueda->vector->mayor){
            zonaBusqueda->vector->mayor = zonaBusqueda->vector->arreglo[i];
            zonaBusqueda->vector->posMayor = i;
        }
    }
    printf("El mayor es %d y la posicion es %d \n", zonaBusqueda->vector->mayor, 
                                                    zonaBusqueda->vector->posMayor);

    pthread_exit(NULL);
}

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <time.h>
// #include <pthread.h> /* POSIX -> gcc -pthread */

// void *buscarMayor (void *);

// int main(){
//     int *pointerC;

//     pointerC = (int*)calloc(20000, sizeof(int));

//     srand(time(NULL));

//     for( int i = 0; i < 20000; i++ ) {
//         pointerC[i] = rand() % 100000;
//     }

//     pthread_t hilos[1]; // creamos 1 hilo

//     pthread_create(&hilos[0], NULL, buscarMayor, pointerC);
//     pthread_join(hilos[0], NULL);

//     return 0;
// }

// void *buscarMayor(void *tmp){
//     int* p = (int *)(tmp);
//     int mayor = 0;
//     int posMayor = 0;
//     for (int i = 0; i < 20000; i++)
//     {
//         if(p[i] > mayor){
//             mayor = p[i];
//             posMayor = i;
//         }
//     }
//     printf("El mayor es %d y la posicion es %d \n", mayor, posMayor);
    
//     pthread_exit(NULL); 
// }