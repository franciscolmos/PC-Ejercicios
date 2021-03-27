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
    int idHiloDelMayor; // ESTE ID LO UTILIZAMOS PARA PODER ACCEDERLO DESDE EL MAIN YA QUE ES COMPARTIDO PARA LOS HILOS Y 
                        // REPRESENTA EL ID DEL HILO QUE ENCONTRO EL MAYOR UNA VEZ QUE TERMINEN TODOS CON SU BUSQUEDA.
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
//  INICIALIZAMOS LOS ATRIBUTOS MAYOR, POSMAYOR Y IDHILODELMAYOR EN -1 PARA VERIFICAR QUE SE ENCUENTRE CORRECTAMENTE
    Vector vector;
    vector.tamano = 0;
    vector.mayor = -1;
    vector.posMayor = -1;
    vector.idHiloDelMayor = -1;

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

//  CREAMOS 2 ZONABUSQUEDA Y LE ASIGNAMOS EL VECTOR Y EL RANGO SOBRE EL QUE DEBE BUSCAR
//  CREAMOS 2 HILOS Y LE ASIGNOS ZONABUSQUEDA A CADA UNO
    ZonaBusqueda zonaBusqueda[2];
    pthread_t hilos[2];
    for (int i = 0; i < 2; i++) {
        zonaBusqueda[i].vector = &vector;
        zonaBusqueda[i].rango.inicio = (vector.tamano * i) / 2;
        zonaBusqueda[i].rango.fin = (vector.tamano * (i+1)) / 2;
        zonaBusqueda[i].id = i+1;
        pthread_create(&hilos[i], NULL, buscarMayor, &zonaBusqueda[i]);
    }

//  ESPERAMOS LA FINALIZACION DE AMBOS HILOS PARA CONTINUAR CON EL MAIN
    for (int i = 0; i < 2; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("El hilo %d encontro el mayor %d en la posicion %d \n", vector.idHiloDelMayor,
                                                                            vector.mayor, 
                                                                            vector.posMayor);

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
            zonaBusqueda->vector->idHiloDelMayor = zonaBusqueda->id; // SE ACTUALIZA EN EL VECTOR EL HILO QUE ENCONTRO EL MAYOR
        }
    }
    pthread_exit(NULL);
}