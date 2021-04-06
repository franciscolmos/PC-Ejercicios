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
    pthread_mutex_t mutex;
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

//  CREACION E INICIALIZACION DEL MUTEX BLOQUEANTE    
    pthread_mutex_init(&vector.mutex, NULL);

//  INGRESO DEL TAMAÑO DEL ARREGLO POR PARTE DEL USUARIO
    while (vector.tamano < 9 || vector.tamano > 40000)
    {
        printf("Ingrese tamano del arreglo (debe ser un valor entre 9 y 40000): ");
        scanf("%d", &vector.tamano);
    }
    
//  SE RESERVA TAMANO ESPACIOS EN MEMRORIA PARA 'ARREGLO' Y SE LA INICIALIZA EN 0
    vector.arreglo = (int*)calloc(vector.tamano, sizeof(int));

//  AGREGAMOS ALEATORIAMENTE VALORES AL ARREGLO, ENTRE 0 Y P-1.
    srand(time(NULL));
    for( int i = 0; i < vector.tamano; i++ ) {
        vector.arreglo[i] = rand() % P;
    }

//  SE INGRESA POR PANTALLA LA CANTIDAD DE HILOS, DEBE SER UN NUMERO ENTRE 2 Y 9 SINO SE PIDE NUEVAMENTE
    int cantHilos = 0;
    while(cantHilos < 2 || cantHilos > 9){
        printf("Ingrese la cantidad de hilos que desee crear (debe ser entre 2 y 9) : ");
        scanf("%d", &cantHilos);
    }

//  CREAMOS cantHilos ZONABUSQUEDA Y LE ASIGNAMOS EL VECTOR Y EL RANGO SOBRE EL QUE DEBE BUSCAR
//  CREAMOS canthilos HILOS Y LE ASIGNOS ZONABUSQUEDA A CADA UNO
    ZonaBusqueda zonaBusqueda[cantHilos];
    pthread_t hilos[cantHilos];
    for (int i = 0; i < cantHilos; i++) {
        zonaBusqueda[i].vector = &vector;
        zonaBusqueda[i].rango.inicio = (vector.tamano * i) / cantHilos;
        zonaBusqueda[i].rango.fin = (vector.tamano * (i+1)) / cantHilos;
        zonaBusqueda[i].id = i+1;
        pthread_create(&hilos[i], NULL, buscarMayor, &zonaBusqueda[i]);
    }

//  ESPERAMOS LA FINALIZACION DE AMBOS HILOS PARA CONTINUAR CON EL MAIN
    for (int i = 0; i < cantHilos; i++) {
        pthread_join(hilos[i], NULL);
    }

//  DEVOLVEMOS POR PANTALLA EL HILO QUE ENCONTRO EL MAYOR, EL MAYOR Y SU POSICION EN EL ARREGLO
    printf("\nEl hilo %d encontro el mayor %d en la posicion %d \n",vector.idHiloDelMayor,
                                                                    vector.mayor, 
                                                                    vector.posMayor);

//  LIBERAMOS EL ESPACIO DE MEMORIA QUE ESTA ASIGNADO POR CALLOC Y APUNTAMOS EL PUNTERO A LA NADA
    free(vector.arreglo);
    vector.arreglo = NULL;
    return 0;
}

//  FUNCION QUE RECIBE *VOID Y LUEGO LO CASTEA A UNA ZONABUSQUEDA PARA PODER HACER LA BUSQUEDA SECUENCIAL DEL MAYOR
void *buscarMayor(void *tmp){
    ZonaBusqueda* zonaBusqueda = (ZonaBusqueda *)(tmp);
    for (int i = zonaBusqueda->rango.inicio; i < zonaBusqueda->rango.fin; i++){
        if(zonaBusqueda->vector->arreglo[i] > zonaBusqueda->vector->mayor){
            pthread_mutex_lock(&zonaBusqueda->vector->mutex);
            printf("\nInicio bloqueo hilo %d\n", zonaBusqueda->id);
            if(zonaBusqueda->vector->arreglo[i] > zonaBusqueda->vector->mayor){
                printf("\n\tHilo %d posible mayor %d\n", zonaBusqueda->id, zonaBusqueda->vector->arreglo[i]);
                zonaBusqueda->vector->mayor = zonaBusqueda->vector->arreglo[i];
                zonaBusqueda->vector->posMayor = i;
                zonaBusqueda->vector->idHiloDelMayor = zonaBusqueda->id;
            }
             // PROTOCOLO DE SALIDA DE LA ZONA CRITICA
            printf("\nFin bloqueo hilo %d\n", zonaBusqueda->id);
            pthread_mutex_unlock(&zonaBusqueda->vector->mutex);
        }
    }
    pthread_exit(NULL);
}