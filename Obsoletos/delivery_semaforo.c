#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

typedef struct{
    sem_t * semaforo1;
    sem_t * semaforo2;
    int * flagNoMasLlamadas;
    int id;
}Sincronizacion;

typedef struct{
    Sincronizacion * sincronizacion;
    sem_t * semaforo3;
}Encargado;

//Funciones de inicializacion de objetos
Sincronizacion * inicializarSincronizacion(sem_t *, sem_t *, int *);
Encargado * inicializarEncargado(Sincronizacion *, sem_t *);

//Funciones de liberacion de memoria
//void liberarSincronizacion(Sincronizacion *);
//void liberarEncargado(Encargado *);
void borrarSemaforo(sem_t *, char *);

//Funciones de telefono
void * gestionTelefono( void *);
void sonando(Sincronizacion *);

//Funciones de encargado
void * gestionEncargado(void *);
void atenderPedido(Encargado *);
void cargarPedido(Encargado *);
//void cobrar(Encargado *);

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
    sem_t * s1 = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * s2 = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * s3 = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * s4 = (sem_t *)(calloc(1, sizeof(sem_t)));

    // Se crea el aviso para indicar que no se aceptan mas llamadas
    int * flag = (int *)(calloc(1, sizeof(int)));
    *flag = 1;

    // Se crean los actores del juego
    Sincronizacion * telefono = inicializarSincronizacion(s1, s2, flag);
    Encargado * encargado = inicializarEncargado(telefono, s4);
    Sincronizacion * cocinero = inicializarSincronizacion(s3, s2, flag);
    Sincronizacion * delivery = inicializarSincronizacion(s4, s3, flag);

    s1 = sem_open("/semTelefono", O_CREAT, O_RDWR, 0);
    s2 = sem_open("/semEncargado", O_CREAT, O_RDWR, 0);
    s3 = sem_open("/semCocinero", O_CREAT, O_RDWR, 0);
    s4 = sem_open("/semDelivery", O_CREAT, O_RDWR, 0);

    // Se instancian las variables hilos de cada objeto
    pthread_t hiloTelefono;
    pthread_t hiloEncargado;
    pthread_t hiloCocinero1;
    pthread_t hiloCocinero2;
    pthread_t hiloCocinero3;
    pthread_t hiloDelivery1;
    pthread_t hiloDelivery2;


    // Se crean los hilos de cada objeto
    pthread_create(&hiloTelefono, NULL, gestionTelefono, (void *)(telefono));
    pthread_create(&hiloEncargado, NULL, gestionEncargado, (void *)(encargado));
    pthread_create(&hiloCocinero1, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloCocinero2, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloCocinero3, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloDelivery1, NULL, gestionDelivery, (void *)(delivery));
    pthread_create(&hiloDelivery2, NULL, gestionDelivery, (void *)(delivery));

    // Se espera que terminen todos los hilos
    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);
    pthread_join(hiloCocinero1, NULL);
    pthread_join(hiloCocinero2, NULL);
    pthread_join(hiloCocinero3, NULL);
    pthread_join(hiloDelivery1, NULL);
    pthread_join(hiloDelivery2, NULL);
    

    // Se libera la memoria de los objetos creados
    //liberarEncargado(encargado);
    free(telefono);
    free(encargado);
    free(cocinero);
    free(delivery);

    int error = 0;

    error = sem_close(s1);
    if(!error){
        error = sem_unlink("/semTelefono");
        if(error){
            perror("sem_unlink()");
        }
    }else{
        perror("sem_close()");
    }

    error = sem_close(s2);
    if(!error){
        error = sem_unlink("/semEmcargado");
        if(error){
            perror("sem_unlink()");
        }
    }else{
        perror("sem_close()");
    }

    error = sem_close(s3);
    if(!error){
        error = sem_unlink("/semCocinero");
        if(error){
            perror("sem_unlink()");
        }
    }else{
        perror("sem_close()");
    }

    error = sem_close(s4);
    if(!error){
        error = sem_unlink("/semDelivery");
        if(error){
            perror("sem_unlink()");
        }
    }else{
        perror("sem_close()");
    }
    
    /*
    sem_close(s2);
    sem_unlink("/semEmcargado");
    sem_close(s3);
    sem_unlink("/semCocinero");
    sem_close(s4);
    sem_unlink("/semDelivery");
    
    borrarSemaforo(s1,);
    borrarSemaforo(s2, "/semEncargado");
    borrarSemaforo(s3, "/semCocinero");
    */

    return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
    Sincronizacion *telefono = (Sincronizacion *) tmp;
    srand(time(NULL));
    printf("-- INICIO FUNCION GESTION TELEFONO --\n");
    for (int i = 0; i < 10; i++)
    {
        int estado = 0;
        sem_getvalue(telefono->semaforo1, &estado);
        while(estado == 1){
            sem_getvalue(telefono->semaforo1, &estado);
        }
        usleep(rand()% 500001 + 500000);
        sonando(telefono);
    }
    *telefono->flagNoMasLlamadas = 0;
    printf("-- FIN FUNCION GESTION TELEFONO --\n");
    pthread_exit(NULL);
}

void sonando(Sincronizacion * telefono) {
    sem_post(telefono->semaforo1);
     printf("\ttelefono sonando\n");
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    printf("-- INICIO FUNCION GESTION ENCARGADO --\n");
    while(*encargado->sincronizacion->flagNoMasLlamadas != 0){
        atenderPedido(encargado);
        cobrar(encargado);
    }
    printf("-- FIN FUNCION GESTION ENCARGADO --\n");
    pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
    int error = 0;
    srand(time(NULL));
    error = sem_trywait(encargado->sincronizacion->semaforo1);
    if(!error){
        printf("\t\ttelefono atendido\n");
        usleep(rand()% 250001 + 500000);
        cargarPedido(encargado);
    }
}

void cargarPedido(Encargado * encargado){
    sem_post(encargado->sincronizacion->semaforo2);
    printf("\t\tpedido cargado\n");
}


void cobrar(Encargado * encargado) {
    int error = 0;
    srand(time(NULL));
    error = sem_trywait(encargado->semaforo3);
    if(!error){
        printf("\t\tdinero guardado en caja\n");
        usleep(rand()% 150001 + 100000);
    }
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
    sem_wait(cocinero->semaforo2);
    printf("\t\t\tcocinando pedido\n");
    usleep(rand()% 1000001 + 1000000);
}
void pedidoCocinado(Sincronizacion * cocinero) {
    sem_post(cocinero->semaforo1);
    printf("\t\t\tpedido cocinado\n");
}

/*
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
    pthread_mutex_lock(delivery->semaforo1);
    printf("\t\t\t\tentregando pedido\n");
    usleep(rand()% 1000001 + 500000);
}
void pedidoEntregado(Sincronizacion * delivery) {
    printf("\t\t\t\tpedido entregado\n");
    srand(time(NULL));
    usleep(rand()% 1000001 + 500000);
    printf("\t\t\t\tllevando dinero a la pizzeria\n");
    pthread_mutex_unlock(delivery->semaforo2);
}
*/

// Inicializacion de un objeto de tipo Sincronizacion
Sincronizacion * inicializarSincronizacion(sem_t *s1, sem_t *s2, int *flag) {
    Sincronizacion * tmp = (Sincronizacion *)(calloc(1, sizeof(Sincronizacion)));
    tmp->semaforo1 = s1;
    tmp->semaforo2 = s2;
    tmp->flagNoMasLlamadas = flag;
    return tmp;
}

// Inicializacion de un objeto de tipo Encargado
Encargado * inicializarEncargado(Sincronizacion * s1, sem_t *s3){
    Encargado * tmp = (Encargado *)(calloc(1, sizeof(Encargado)));
    tmp->sincronizacion = s1;
    tmp->semaforo3 = s3;
    return tmp;
}

void borrarSemaforo(sem_t * semaforo, char * nombreArchivo){
    int error = 0;
    error=sem_close(semaforo);
      if (error) {
	perror("sem_close()");
      }
      else {
	printf("Semaforo cerrado\n");
      }
    
    if (!error) {
      error = sem_unlink(nombreArchivo);
      if (error) {
	perror("sem_unlink()");
      }
      else {
	printf("Semaforo borrado!\n");
      }
    }
}

/*
// Liberacion de memoria de la sincronizacion
void liberarSincronizacion(Sincronizacion* tmp){
    free(tmp->flagNoMasLlamadas);
    pthread_mutex_destroy(tmp->semaforo1);
    pthread_mutex_destroy(tmp->semaforo2);
}

// Liberacion de memoria del encargado, que usa la liberacion de memoria de la sincronizacion
void liberarEncargado(Encargado * tmp){
    liberarSincronizacion(tmp->sincronizacion);
    pthread_mutex_destroy(tmp->mutex3);
}
*/