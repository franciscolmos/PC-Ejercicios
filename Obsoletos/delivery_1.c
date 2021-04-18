#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

typedef struct{
    pthread_mutex_t *mutexEncargado;
    pthread_mutex_t *mutexTelefono;
    pthread_mutex_t *mutexDelivery;
    pthread_mutex_t *mutexCocinero;
    int * flagNoMasLlamadas;
}Sincronizacion;

//Funciones de inicializacion de objetos
Sincronizacion * inicializarSincronizacion(pthread_mutex_t*, pthread_mutex_t*, 
                                           pthread_mutex_t*, pthread_mutex_t*, int*);
void liberarSincronizacion(Sincronizacion *);

//Funciones de telefono
void * gestionTelefono( void *);
void sonando(Sincronizacion *);
void telefonoAtendido(Sincronizacion *);

//Funciones de encargado
void * gestionEncargado(void *);
void atenderPedido(Sincronizacion *);
void cargarPedido(Sincronizacion *);
void cobrar(Sincronizacion *);

//Funciones de cocinero
void * gestionCocinero(void *);
void nuevoPedido(Sincronizacion *);
void pedidoCocinado(Sincronizacion *);

//Funciones de delivery
void * gestionDelivery(void *);
void entregarPedido(Sincronizacion *);
void pedidoEntregado(Sincronizacion *);

int main(){
    // Creamos los mutex de forma dinamica
    pthread_mutex_t * m1 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m2 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m3 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m4 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));

    // Se crea el aviso para indicar que no se aceptan mas llamadas
    int * flag = (int *)(calloc(1, sizeof(int)));
    *flag = 1;

    // Se crean los actores del juego
    Sincronizacion * telefono = inicializarSincronizacion(m1, m2, m3, m4, flag);
    Sincronizacion * encargado = inicializarSincronizacion(m1, m2, m3, m4, flag);
    Sincronizacion * cocinero = inicializarSincronizacion(m1, m2, m3, m4, flag);
    Sincronizacion * delivery = inicializarSincronizacion(m1, m2, m3, m4, flag);

    // Se inician los mutex
    pthread_mutex_init(m1, NULL);
    pthread_mutex_init(m2, NULL);
    pthread_mutex_init(m3, NULL);
    pthread_mutex_init(m4, NULL);

    // Encargado, cocinero y delivery comienzan bloqueados
    pthread_mutex_lock(m2);
    pthread_mutex_lock(m3);
    pthread_mutex_lock(m4);

    // Se instancian las variables hilos de cada objeto
    pthread_t hiloTelefono;
    pthread_t hiloEncargado;
    pthread_t hiloCocinero;
    pthread_t hiloDelivery;

    // Se crean los hilos de cada objeto
    pthread_create(&hiloTelefono, NULL, gestionTelefono, (void *)(telefono));
    pthread_create(&hiloEncargado, NULL, gestionEncargado, (void *)(encargado));
    pthread_create(&hiloCocinero, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloDelivery, NULL, gestionDelivery, (void *)(delivery));

    // Se espera que terminen todos los hilos
    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);
    pthread_join(hiloCocinero, NULL);
    pthread_join(hiloDelivery, NULL);

    // Se libera la memoria de los objetos creados
    liberarSincronizacion(telefono);
    free(telefono);
    free(encargado);
    free(cocinero);
    free(delivery);

    return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
    Sincronizacion *telefono = (Sincronizacion *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION TELEFONO --\n");
    for (int i = 0; i < 10; i++)
    {
        usleep(rand()% 500001 + 500000);
        sonando(telefono);
        telefonoAtendido(telefono);
    }
    *telefono->flagNoMasLlamadas = 0;
    printf("-- FIN FUNCION GESTION TELEFONO --\n");
    pthread_exit(NULL);
}

void sonando(Sincronizacion * telefono) {
    pthread_mutex_lock(telefono->mutexTelefono);
    printf("\ttelefono sonando\n");
}

void telefonoAtendido(Sincronizacion * telefono) {
    pthread_mutex_unlock(telefono->mutexEncargado);
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
    Sincronizacion *encargado = (Sincronizacion *) tmp;
    printf("-- INICIO FUNCION GESTION ENCARGADO --\n");
    while(*encargado->flagNoMasLlamadas != 0){
        atenderPedido(encargado);
        cargarPedido(encargado);
        cobrar(encargado);
    }
    printf("-- FIN FUNCION GESTION ENCARGADO --\n");
    pthread_exit(NULL);
}

void atenderPedido(Sincronizacion * encargado){
    srand(time(NULL));
    pthread_mutex_lock(encargado->mutexEncargado);
    printf("\t\ttelefono atendido\n");
    usleep(rand()% 250001 + 500000);
}

void cargarPedido(Sincronizacion * encargado){
    printf("\t\tpedido cargado\n");
    pthread_mutex_unlock(encargado->mutexCocinero);
}

void cobrar(Sincronizacion * encargado) {
    srand(time(NULL));
    pthread_mutex_lock(encargado->mutexEncargado);
    printf("\t\tdinero guardado en caja\n");
    usleep(rand()% 150001 + 100000);
    pthread_mutex_unlock(encargado->mutexTelefono);
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
    Sincronizacion *cocinero = (Sincronizacion *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION COCINERO --\n");
    while(*cocinero->flagNoMasLlamadas != 0){
        nuevoPedido(cocinero);
        pedidoCocinado(cocinero);
    }
    printf("-- FIN FUNCION GESTION COCINERO --\n");
    pthread_exit(NULL);
}
void nuevoPedido(Sincronizacion * cocinero) {
    srand(time(NULL));
    pthread_mutex_lock(cocinero->mutexCocinero);
    printf("\t\t\tcocinando pedido\n");
    usleep(rand()% 1000001 + 1000000);
}
void pedidoCocinado(Sincronizacion * cocinero) {
    printf("\t\t\tpedido cocinado\n");
    pthread_mutex_unlock(cocinero->mutexDelivery);
}

// Hilo Delivery
void * gestionDelivery(void * tmp) {
    Sincronizacion *delivery = (Sincronizacion *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION DELIVERY --\n");
    while(*delivery->flagNoMasLlamadas != 0){
        entregarPedido(delivery);
        pedidoEntregado(delivery);
    }
    printf("-- FIN FUNCION GESTION DELIVERY --\n");
    pthread_exit(NULL);
}
void entregarPedido(Sincronizacion * delivery) {
    srand(time(NULL));
    pthread_mutex_lock(delivery->mutexDelivery);
    printf("\t\t\t\tentregando pedido\n");
    usleep(rand()% 1000001 + 500000);
}
void pedidoEntregado(Sincronizacion * delivery) {
    printf("\t\t\t\tpedido entregado\n");
    srand(time(NULL));
    usleep(rand()% 1000001 + 500000);
    printf("\t\t\t\tllevando dinero a la pizzeria\n");
    pthread_mutex_unlock(delivery->mutexEncargado);
}

// Inicializacion de un objeto de tipo Sincronizacion
Sincronizacion * inicializarSincronizacion(pthread_mutex_t* m1, pthread_mutex_t* m2, 
                                           pthread_mutex_t* m3, pthread_mutex_t* m4, int* flag) {
    Sincronizacion * tmp = (Sincronizacion *)(calloc(1, sizeof(Sincronizacion)));
    tmp->mutexTelefono = m1;
    tmp->mutexEncargado = m2;
    tmp->mutexCocinero = m3;
    tmp->mutexDelivery = m4;
    tmp->flagNoMasLlamadas = flag;
    return tmp;
}

// Liberacion de memoria
void liberarSincronizacion(Sincronizacion* tmp){
    free(tmp->flagNoMasLlamadas);
    pthread_mutex_destroy(tmp->mutexCocinero);
    pthread_mutex_destroy(tmp->mutexEncargado);
    pthread_mutex_destroy(tmp->mutexDelivery);
    pthread_mutex_destroy(tmp->mutexTelefono);
}