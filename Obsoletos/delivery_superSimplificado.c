#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

typedef struct{
    pthread_mutex_t * mutex1;
    pthread_mutex_t * mutex2;
    int * flagNoMasLlamadas;
}Sincronizacion;

typedef struct{
    Sincronizacion * sincronizacion;
    pthread_mutex_t * mutex3;
}Encargado;

enum cambiarMutex {BLOQUEAR, DESBLOQUEAR};

//Funciones de inicializacion de objetos
Sincronizacion * inicializarSincronizacion(pthread_mutex_t *, pthread_mutex_t *, int *);
Encargado * inicializarEncargado(Sincronizacion *, pthread_mutex_t *);

//Funciones de liberacion de memoria
void liberarSincronizacion(Sincronizacion *);
void liberarEncargado(Encargado *);

//Funciones de mutex
void usarMutex(pthread_mutex_t *, pthread_mutex_t *, int, int, char *);

void * gestionTelefono( void *);
void * gestionEncargado(void *);
void * gestionCocinero(void *);
void * gestionDelivery(void *);

int main(){
    srand(time(NULL));

    // Creamos los mutex de forma dinamica
    pthread_mutex_t * m1 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m2 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m3 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));
    pthread_mutex_t * m4 = (pthread_mutex_t *)(calloc(1, sizeof(pthread_mutex_t)));

    // Se crea el aviso para indicar que no se aceptan mas llamadas
    int * flag = (int *)(calloc(1, sizeof(int)));
    *flag = 1;

    // Se crean los actores del juego
    Sincronizacion * telefono = inicializarSincronizacion(m1, m2, flag);
    Encargado * encargado = inicializarEncargado(telefono, m3);
    Sincronizacion * cocinero = inicializarSincronizacion(m3, m4, flag);
    Sincronizacion * delivery = inicializarSincronizacion(m4, m2, flag);

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
    liberarEncargado(encargado);
    free(telefono);
    free(encargado);
    free(cocinero);
    free(delivery);

    return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
    Sincronizacion *telefono = (Sincronizacion *) tmp;
    for (int i = 0; i < 10; i++) {
        usleep(rand()% 500001 + 500000);
        usarMutex(telefono->mutex1, telefono->mutex2, 1, 0, "\ttelefono atendido\n");
    }
    *telefono->flagNoMasLlamadas = 0;
    pthread_exit(NULL);
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    while(*encargado->sincronizacion->flagNoMasLlamadas != 0){
        usarMutex(encargado->sincronizacion->mutex2, encargado->mutex3, 
                  250001, 500000, "\t\tpedido cargado\n");

        usarMutex(encargado->sincronizacion->mutex2, encargado->sincronizacion->mutex1,
                  150001, 100000, "\t\tdinero guardado en caja\n");
    }
    pthread_exit(NULL);
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
    Sincronizacion *cocinero = (Sincronizacion *) tmp;
    while(*cocinero->flagNoMasLlamadas != 0)
        usarMutex(cocinero->mutex1, cocinero->mutex2, 1000001, 1000000, 
                  "\t\t\tpedido cocinado\n");
    pthread_exit(NULL);
}

// Hilo Delivery
void * gestionDelivery(void * tmp) {
    Sincronizacion *delivery = (Sincronizacion *) tmp;
    while(*delivery->flagNoMasLlamadas != 0)
        usarMutex(delivery->mutex1, delivery->mutex2, 2000001, 500000, 
                  "\t\t\t\tpedido entregado\n");
    pthread_exit(NULL);
}

// Inicializacion de un objeto de tipo Sincronizacion
Sincronizacion * inicializarSincronizacion(pthread_mutex_t *m1, pthread_mutex_t *m2, int *flag) {
    Sincronizacion * tmp = (Sincronizacion *)(calloc(1, sizeof(Sincronizacion)));
    tmp->mutex1 = m1;
    tmp->mutex2 = m2;
    tmp->flagNoMasLlamadas = flag;
    return tmp;
}

// Inicializacion de un objeto de tipo Encargado
Encargado * inicializarEncargado(Sincronizacion * s1, pthread_mutex_t *m1){
    Encargado * tmp = (Encargado *)(calloc(1, sizeof(Encargado)));
    tmp->sincronizacion = s1;
    tmp->mutex3 = m1;
    return tmp;
}

void usarMutex(pthread_mutex_t * mutexBloq, pthread_mutex_t * mutexDesbloq, int rango, 
               int base, char * mensaje) {
    pthread_mutex_lock(mutexBloq);
    usleep(rand()% rango + base);
    printf(mensaje);
    pthread_mutex_unlock(mutexDesbloq);
}

// Liberacion de memoria de la sincronizacion
void liberarSincronizacion(Sincronizacion* tmp){
    free(tmp->flagNoMasLlamadas);
    pthread_mutex_destroy(tmp->mutex1);
    pthread_mutex_destroy(tmp->mutex2);
}

// Liberacion de memoria del encargado, que usa la liberacion de memoria de la sincronizacion
void liberarEncargado(Encargado * tmp){
    liberarSincronizacion(tmp->sincronizacion);
    pthread_mutex_destroy(tmp->mutex3);
}