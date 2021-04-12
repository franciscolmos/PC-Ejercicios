#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

typedef struct{
    pthread_mutex_t *mutexCocinero;
    int * flagNoMasLlamadas;
}Cocinero;

typedef struct{
    pthread_mutex_t *mutexEncargado;
    pthread_mutex_t *mutexTelefono;
    int * flagNoMasLlamadas;
}Encargado;

typedef struct{
    pthread_mutex_t *mutexEncargado;
    pthread_mutex_t *mutexTelefono;
    int * flagNoMasLlamadas;
}Telefono;

typedef struct {
    pthread_mutex_t *mutexDelivery;
    int * flagNoMasLlamadas;
}Delivery;

//Esta funcion se encarga de sonar cada cierto tiempo solo si el mutex no esta tomado
void * sonar( void *);
void * gestionPedidos(void *);
void atenderPedido(Encargado *);
void cargarPedido(Encargado *);

int main(){
    //Se crean los mutex
    //pthread_mutex_t * m1 = (pthread_mutex_t *) (calloc(1,sizeof(pthread_mutex_t)));
    //pthread_mutex_t * m2 = (pthread_mutex_t *) (calloc(1,sizeof(pthread_mutex_t)));
    
    pthread_mutex_t m1;
    pthread_mutex_t m2;

    //Se crea el aviso para indicar que no se aceptan mas llamadas
    int * flag = (int *)(calloc(1, sizeof(int)));
    *flag = 1;

    //Se crea un objeto telefono
    Telefono * telefono;
    telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
    telefono->mutexTelefono = &m1;
    telefono->mutexEncargado = &m2;
    telefono->flagNoMasLlamadas = flag;

    //Se crea un objeto encargado
    Encargado * encargado;
    encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
    encargado->mutexEncargado = &m2;
    encargado->mutexTelefono = &m1;
    encargado->flagNoMasLlamadas = flag;

    //Se inician los mutex
    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);

    //El encargado empieza bloqueado y el telefono desbloqueado
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m1);

    //Se instancian las variables hilos de cada objeto
    pthread_t hiloTelefono;
    pthread_t hiloEncargado;

    //Se crean los hilos de cada objeto
    pthread_create(&hiloTelefono, NULL, sonar, (void *)(telefono));
    pthread_create(&hiloEncargado, NULL, gestionPedidos, (void *)(encargado));

    //Se espera que terminen todos los hilos
    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);

    //Se destruyen los mutex para liberar memoria
    pthread_mutex_destroy(&m2);
    pthread_mutex_destroy(&m1);

    //Se destruyen los objeto creados pra liberar memoria
    free(flag);
    free(telefono);
    free(encargado);

    return 0;
}

void * sonar(void * tmp){
    Telefono *telefono = (Telefono *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION SONAR --\n");
    for (int i = 0; i < 10; i++)
    {
        usleep(rand()% 501 + 500);
        pthread_mutex_lock(telefono->mutexTelefono);
        printf("\ttelefono sonando %d\n", i+1);
        pthread_mutex_unlock(telefono->mutexEncargado);
    }
    *telefono->flagNoMasLlamadas = 0;
    printf("-- FIN FUNCION SONAR --\n");
    pthread_exit(NULL);
}

void * gestionPedidos(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION PEDIDOS --\n");
    while(*encargado->flagNoMasLlamadas != 0){
        atenderPedido(encargado);
        usleep(rand()% 251 + 500);
        cargarPedido(encargado);
    }
    printf("-- FIN FUNCION GESTION PEDIDOS --\n");
    pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
    pthread_mutex_lock(encargado->mutexEncargado);
    printf("\ttelefono atendido\n");
}

void cargarPedido(Encargado * encargado){
    printf("\tpedido cargado\n");
    pthread_mutex_unlock(encargado->mutexTelefono);
}