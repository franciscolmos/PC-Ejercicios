#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

int corte = 0;

typedef struct{
    sem_t * semaforo1;
    sem_t * semaforo2;
}Sincronizacion;

typedef struct{
    sem_t * semaforo1;
    sem_t * semaforo2;
    sem_t * semaforo3;
}Delivery;

typedef struct{
    Sincronizacion * sincro;
    sem_t * semaforo3;
    sem_t * semaforo4;
}Encargado;

//Funciones de inicializacion de objetos
Sincronizacion * inicializarSincronizacion(sem_t *, sem_t *);
Encargado * inicializarEncargado(Sincronizacion *, sem_t *, sem_t *);
Delivery * inicializarDelivery(sem_t *, sem_t *, sem_t *);

//Funciones de liberacion de memoria
void borrarSemaforo(sem_t *, sem_t *, sem_t *, sem_t *, sem_t *, sem_t *);

//Funciones de telefono
void * gestionTelefono( void *);
void sonando(Sincronizacion *);

//Funciones de encargado
void * gestionEncargado(void *);
void atenderPedido(Encargado *);
void cargarPedido(Encargado *);

//Funciones de cocinero
void * gestionCocinero(void *);
void nuevoPedido(Sincronizacion *);
void pedidoCocinado(Sincronizacion *);

//Funciones de delivery
void * gestionDelivery(void *);
void consultarPedidos(Delivery *);
void entregarPedido(Delivery *);
void entregarDinero(Delivery *, int);

int main(){
    srand(time(NULL));

    // Creamos los mutex de forma dinamica
    sem_t * semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoEncargado = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoCocineros = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoPedidos = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoDeliverys = (sem_t *)(calloc(1, sizeof(sem_t)));

    semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
    semaforoLlamadas = sem_open("/semLlamada", O_CREAT, O_RDWR, 0);
    semaforoEncargado = sem_open("/semEncargado", O_CREAT, O_RDWR, 1);
    semaforoCocineros = sem_open("/semCocinero", O_CREAT, O_RDWR, 0);
    semaforoPedidos = sem_open("/semPedidos", O_CREAT, O_RDWR, 0);
    semaforoDeliverys = sem_open("/semDelivery", O_CREAT, O_RDWR, 0);

    // Se crean los actores del juego
    Sincronizacion * telefono = inicializarSincronizacion(semaforoTelefono, semaforoLlamadas);
    Encargado * encargado = inicializarEncargado(telefono, semaforoEncargado, semaforoCocineros);
    Sincronizacion * cocinero = inicializarSincronizacion(semaforoCocineros, semaforoPedidos);
    Delivery * delivery = inicializarDelivery(semaforoPedidos, semaforoDeliverys, semaforoEncargado);

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

    sleep(20);
    corte = 1;

    // Se espera que terminen todos los hilos
    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);
    pthread_join(hiloCocinero1, NULL);
    pthread_join(hiloCocinero2, NULL);
    pthread_join(hiloCocinero3, NULL);
    pthread_join(hiloDelivery1, NULL);
    pthread_join(hiloDelivery2, NULL);

    // Se libera la memoria de los objetos creados
    free(telefono);
    free(encargado);
    free(cocinero);
    free(delivery);

    borrarSemaforo(semaforoTelefono, semaforoLlamadas, semaforoEncargado, semaforoCocineros, semaforoPedidos, semaforoDeliverys);

    return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
    Sincronizacion *telefono = (Sincronizacion *) tmp;
    //printf("-- INICIO FUNCION GESTION TELEFONO --\n");
    for (int i = 0; i < 12; i++) {
        sem_wait(telefono->semaforo1); //VERIFICA QUE NO ESTE EN LLAMADA
        usleep(rand()% 250001 + 250000);
        sonando(telefono);
    }
    //printf("-- FIN FUNCION GESTION TELEFONO --\n");
    pthread_exit(NULL);
}

void sonando(Sincronizacion * telefono) {
    printf("\ttelefono sonando\n");
    sem_post(telefono->semaforo2); //AVISA QUE ESTA SONANDO
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    //printf("-- INICIO FUNCION GESTION ENCARGADO --\n");
    while(corte != 1){
        int error = 0;
        error = sem_trywait(encargado->semaforo3); //VERIFICA QUE NO ESTE COBRANDO
        if(!error)
            atenderPedido(encargado);
        usleep(500000);
    }
    //printf("-- FIN FUNCION GESTION ENCARGADO --\n");
    pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
    int error = 0;
    error = sem_trywait(encargado->sincro->semaforo2); //VERIFICA SI HAY ALGUNA LLAMADA ENTRANTE
    if(!error){
        printf("\t\ttelefono atendido\n");
        usleep(rand()% 250001 + 250000);
        printf("\t\ttelefono colgado\n");
        sem_post(encargado->sincro->semaforo1); //CORTA LA LLAMADA
        cargarPedido(encargado);
    }
    sem_post(encargado->semaforo3); //SE LIBERA
}

void cargarPedido(Encargado * encargado){
    usleep(rand()% 125001 + 125000);
    printf("\t\tpedido cargado\n");
    sem_post(encargado->semaforo4); //AVISA A LOS COCINEROS QUE TIENEN OTRO PEDIDO
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
    Sincronizacion *cocinero = (Sincronizacion *) tmp;
    //printf("-- INICIO FUNCION GESTION COCINERO --\n");
    while(corte != 1)
        nuevoPedido(cocinero);
    //printf("-- FIN FUNCION GESTION COCINERO --\n");
    pthread_exit(NULL);
}

void nuevoPedido(Sincronizacion * cocinero) {
    srand(time(NULL));
    int error = 0;
    error = sem_trywait(cocinero->semaforo1); //ESPERAN QUE LLEGUE UN PEDIDO
    if(!error) {
        printf("\t\t\tcocinando pedido\n");
        usleep(rand()% 1000001 + 1000000);
        pedidoCocinado(cocinero);
    }
}

void pedidoCocinado(Sincronizacion * cocinero) {
    printf("\t\t\tpedido cocinado\n");
    sem_post(cocinero->semaforo2); //AVISAN AL DELIVERY QUE EL PEDIDO ESTA LISTO
}


// Hilo Delivery
void * gestionDelivery(void * tmp) {
    Delivery *delivery = (Delivery *) tmp;
    //printf("-- INICIO FUNCION GESTION DELIVERY --\n");
    while(corte != 1){
        consultarPedidos(delivery);
    }
    //printf("-- FIN FUNCION GESTION DELIVERY --\n");
    pthread_exit(NULL);
}

void consultarPedidos(Delivery * delivery) {
    int error = 0;
    error = sem_trywait(delivery->semaforo1); //VERIFICA SI HAY ALGUN PEDIDO PARA LLEVAR
    if(!error)
        entregarPedido(delivery);
    int cantPedidosEntregar = 0;
    sem_getvalue(delivery->semaforo2, &cantPedidosEntregar);
    if(cantPedidosEntregar > 0){
        entregarDinero(delivery, cantPedidosEntregar);
    }
}

void entregarPedido(Delivery * delivery) {
    printf("\t\t\t\tentregando pedido\n");
    usleep(rand()% 1000001 + 500000);
    printf("\t\t\t\tvolviendo a pizzeria\n");
    usleep(rand()% 1000001 + 500000);
    sem_post(delivery->semaforo2); //AVISA QUE TIENE DINERO DE OTRO PEDIDO
}

void entregarDinero(Delivery * delivery, int cantPedidosEntregar) {
    int error = 0;
    error = sem_trywait(delivery->semaforo3); //INTENTA DARLE DINERO AL ENCARGADO
    if(!error){
        usleep(rand()% 100001 + (cantPedidosEntregar * 250000));
        printf("\t\t\t\tDinero en caja de %d pedidos\n", cantPedidosEntregar);
        for(int i = 0; i < cantPedidosEntregar; i++) {
            error = sem_trywait(delivery->semaforo2);
        }
        sem_post(delivery->semaforo3);
    }
}

// Inicializacion de un objeto de tipo Sincronizacion
Sincronizacion * inicializarSincronizacion(sem_t *s1, sem_t *s2) {
    Sincronizacion * tmp = (Sincronizacion *)(calloc(1, sizeof(Sincronizacion)));
    tmp->semaforo1 = s1;
    tmp->semaforo2 = s2;
    return tmp;
}

// Inicializacion de un objeto de tipo Delivery
Delivery * inicializarDelivery(sem_t *s1, sem_t *s2, sem_t *s3) {
    Delivery * tmp = (Delivery *)(calloc(1, sizeof(Delivery)));
    tmp->semaforo1 = s1;
    tmp->semaforo2 = s2;
    tmp->semaforo3 = s3;
    return tmp;
}

// Inicializacion de un objeto de tipo Encargado
Encargado * inicializarEncargado(Sincronizacion *sincro, sem_t *s3, sem_t *s4){
    Encargado * tmp = (Encargado *)(calloc(1, sizeof(Encargado)));
    tmp->sincro = sincro;
    tmp->semaforo3 = s3;
    tmp->semaforo4 = s4;
    return tmp;
}

void borrarSemaforo(sem_t * s1, sem_t * s2, sem_t * s3, sem_t * s4, sem_t * s5, sem_t * s6){
    int error = 0;
    error = sem_close(s1);
    if(!error){
        error = sem_unlink("/semTelefono");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }
    }else{
        perror("sem_close()");
    }

    error = sem_close(s2);
    if(!error){
        error = sem_unlink("/semLlamada");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }        
    }else{
        perror("sem_close()");
    }

    error = sem_close(s3);
    if(!error){
        error = sem_unlink("/semEncargado");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }        
    }else{
        perror("sem_close()");
    }

    error = sem_close(s4);
    if(!error){
        error = sem_unlink("/semCocinero");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }        
    }else{
        perror("sem_close()");
    }

    error = sem_close(s5);
    if(!error){
        error = sem_unlink("/semPedidos");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }        
    }else{
        perror("sem_close()");
    }

    error = sem_close(s6);
    if(!error){
        error = sem_unlink("/semDelivery");
        if(error){
            perror("sem_unlink()");
        }
        else {
            printf("Semaforo borrado!\n");
        }        
    }else{
        perror("sem_close()");
    }
}