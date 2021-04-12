#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

int termino = 1;

typedef struct{
    pthread_mutex_t *mutexCocinero;
}Cocinero;

typedef struct{
    pthread_mutex_t *mutexEncargado;
    pthread_mutex_t *mutexTelefono;
}Encargado;

typedef struct{
    pthread_mutex_t *mutexEncargado;
    pthread_mutex_t *mutexTelefono;
}Telefono;

typedef struct {
    pthread_mutex_t *mutexDelivery;
}Delivery;

//Esta funcion se encarga de sonar cada cierto tiempo solo si el mutex no esta tomado
void * sonar( void *);
void * gestionPedidos(void *);
void atenderPedido(Encargado *);
void cargarPedido(Encargado *);

int main(){
    pthread_mutex_t m1;
    pthread_mutex_t m2;

    Telefono telefono;
    telefono.mutexTelefono = &m1;
    telefono.mutexEncargado = &m2;

    Encargado encargado;
    encargado.mutexEncargado = &m2;
    encargado.mutexTelefono = &m1;

    pthread_mutex_init(telefono.mutexTelefono, NULL);
    pthread_mutex_init(encargado.mutexEncargado, NULL);
    //EL encargado empieza bloqueado
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m1);

    pthread_t hiloTelefono;
    pthread_t hiloEncargado;

    pthread_create(&hiloTelefono, NULL, sonar, &telefono);
    pthread_create(&hiloEncargado, NULL, gestionPedidos, &encargado);

    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);

    pthread_mutex_destroy(&m2);
    pthread_mutex_destroy(&m1);

    return 0;
}

void * sonar(void * tmp){
    Telefono *telefono = (Telefono *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION SONAR --\n");
    for (int i = 0; i < 10; i++)
    {  
        printf("\ttelefono libre\n");
        usleep(rand()% 501 + 500);
        printf("\ttelefono sonando %d\n", i+1);
        pthread_mutex_lock(&telefono->mutexTelefono);
        pthread_mutex_unlock(&telefono->mutexEncargado);
        printf("\ttelefono atendido\n");
    }
    printf("-- FIN FUNCION SONAR --\n");
    termino = 0;
    pthread_exit(NULL);
}

void * gestionPedidos(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION PEDIDOS --\n");
    while(termino){
        atenderPedido(encargado);
        usleep(rand()% 251 + 500);
        cargarPedido(encargado);
    }
    printf("-- FIN FUNCION GESTION PEDIDOS --\n");
    pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
    printf("asdasds\n");
    pthread_mutex_lock(&encargado->mutexEncargado);
    printf("asdasdsasfdsgfsdgdfg\n");
}

void cargarPedido(Encargado * encargado){
    printf("\tpedido cargado\n");
    pthread_mutex_unlock(&encargado->mutexTelefono);
}