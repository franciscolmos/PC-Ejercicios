#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

#define PEDIDOS 8
#define COCINEROS 3

//float precios[8] = {350, 200, 300, 400, 375, 275, 300, 0};

// ESTRUCTURA DEL PEDIDO
typedef struct{
    int id;
    int precio;
}Pedido;

// BUFFER CIRUCULAR PARA ALMACENAR PEDIDOS
struct boundedBuffer_t{
  int inicio, fin; // fin es el proximo valor libre a escribir, e inicio es el lugar del proximo dato a leer
  Pedido buf[PEDIDOS];
  sem_t *lleno, *vacio, *leyendo, *escribiendo; //leyendo y escribiendo son semaforos binarios, solo me indican si se esta leyendo o escribiendo. En cambio
};

// ESTRUCTURA DEL TELEFONO
typedef struct{
    sem_t * semaforoTelefono;
    sem_t * semaforoLlamadas;
    Pedido * pedido;
}Telefono;

// ESTRUCTURA DEL COCINERO
typedef struct{
    struct boundedBuffer_t * boundedBufferPedidos;
}Cocinero;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
    Telefono * telefono;
    struct boundedBuffer_t * boundedBufferPedidos;
}Encargado;

//Funciones de inicializacion de objetos
void crearSemaforosBoundedBuffer(struct boundedBuffer_t *);
Telefono * inicializarTelefono(sem_t *, sem_t *, Pedido *);
Encargado * inicializarEncargado(Telefono *, struct boundedBuffer_t *);
Cocinero * inicializarCocinero(struct boundedBuffer_t *);

//Funciones de liberacion de memoria
void borrarSemaforo(sem_t *, sem_t *);
int borrarSemaforosBoundedBuffer(struct boundedBuffer_t *);

//Funciones de telefono
void * gestionTelefono( void *);

//Funciones de encargado
void * gestionEncargado(void *);
void atenderPedido(Encargado *);
void cargandoPedido(Encargado *, int);

//Funciones de cocinero
void * gestionCocinero(void *);
void cocinarPedido(Cocinero *);
void pedidoCocinado(Cocinero *);

int main(){
    srand(time(NULL));

    // Creamos un pedido
    Pedido * pedido = (Pedido *)(calloc(1, sizeof(Pedido)));

    // Creamos un bbPedidos
    struct boundedBuffer_t * boundedBufferPedidos = (struct boundedBuffer_t *)calloc(1, sizeof(struct boundedBuffer_t));
    boundedBufferPedidos->inicio = 0;
    boundedBufferPedidos->fin    = 0;
    crearSemaforosBoundedBuffer(boundedBufferPedidos);

    // Creamos los semaforos de forma dinamica
    sem_t * semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
    sem_t * semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));

    semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
    semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);

    // Se crean los actores del juego
    Telefono * telefono = inicializarTelefono(semaforoTelefono, semaforoLlamadas, pedido);
    Encargado * encargado = inicializarEncargado(telefono, boundedBufferPedidos);
    Cocinero * cocinero = inicializarCocinero(boundedBufferPedidos);

    // Se instancian las variables hilos de cada objeto
    pthread_t hiloTelefono;
    pthread_t hiloEncargado;
    pthread_t hiloCocinero1;
    pthread_t hiloCocinero2;
    pthread_t hiloCocinero3;

    // Se crean los hilos de cada objeto
    pthread_create(&hiloTelefono, NULL, gestionTelefono, (void *)(telefono));
    pthread_create(&hiloEncargado, NULL, gestionEncargado, (void *)(encargado));
    pthread_create(&hiloCocinero1, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloCocinero2, NULL, gestionCocinero, (void *)(cocinero));
    pthread_create(&hiloCocinero3, NULL, gestionCocinero, (void *)(cocinero));

    // Se espera que terminen todos los hilos
    pthread_join(hiloTelefono, NULL);
    pthread_join(hiloEncargado, NULL);
    pthread_join(hiloCocinero1, NULL);
    pthread_join(hiloCocinero2, NULL);
    pthread_join(hiloCocinero3, NULL);

    // Se libera la memoria de los objetos creados
    free(telefono);
    free(encargado);
    free(cocinero);
    free(pedido);

    borrarSemaforo(semaforoTelefono, semaforoLlamadas);
    borrarSemaforosBoundedBuffer(boundedBufferPedidos);
    return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
    Telefono *telefono = (Telefono *) tmp;
    for (int i = 0; i < PEDIDOS; i++) {
        sem_wait(telefono->semaforoTelefono);
        usleep(rand()% 1000001 + 1000000);
        if(i == (PEDIDOS - 1)){
            telefono->pedido->id = -1; 

        }else{
            telefono->pedido->id = i;
        }
        sem_post(telefono->semaforoLlamadas);
        printf("\ttelefono sonando\n");
    }
    printf("hilto telefono cerrado\n");
    pthread_exit(NULL);
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
    Encargado *encargado = (Encargado *) tmp;
    while(encargado->telefono->pedido->id != -1){
        atenderPedido(encargado);
        usleep(500000);
    }
    atenderPedido(encargado);
    printf("hilto encargado cerrado\n");
    pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
    int error = 0;
    error = sem_trywait(encargado->telefono->semaforoLlamadas); //VERIFICA SI HAY ALGUNA LLAMADA ENTRANTE
    if(!error){
        printf("\t\ttelefono atendido\n");
        usleep(rand()% 250001 + 250000);
        int codigoPedido = encargado->telefono->pedido->id;
        printf("\t\ttelefono colgado\n");
        sem_post(encargado->telefono->semaforoTelefono);
        if(codigoPedido == -1){
        
            for (int i = 0; i < COCINEROS-1; i++)
            {
                cargandoPedido(encargado, codigoPedido);
            }
        
        }
        cargandoPedido(encargado, codigoPedido);
    }
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
    Cocinero *cocinero = (Cocinero *) tmp;
    while(cocinero->boundedBufferPedidos->buf[cocinero->boundedBufferPedidos->inicio].id != -1){
        cocinarPedido(cocinero);
    }
    printf("hilto cocinero cerrado");
    pthread_exit(NULL);
}

// Inicializacion de un objeto de tipo Sincronizacion
Telefono * inicializarTelefono(sem_t *semaforoTelefono, sem_t *semaforoLlamada, Pedido * pedido) {
    Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
    telefono->semaforoTelefono = semaforoTelefono;
    telefono->semaforoLlamadas = semaforoLlamada;
    telefono-> pedido = pedido;
    return telefono;
}

// Inicializacion de un objeto de tipo Encargado
Encargado * inicializarEncargado(Telefono *telefono, struct boundedBuffer_t * boundedBufferPedidos ){
    Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
    encargado->telefono = telefono;
    encargado->boundedBufferPedidos = boundedBufferPedidos;
    return encargado;
}

Cocinero * inicializarCocinero(struct boundedBuffer_t * boundedBufferPedidos) {
    Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
    cocinero->boundedBufferPedidos = boundedBufferPedidos;
    return cocinero;
}

void borrarSemaforo(sem_t * s1, sem_t * s2){
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
        error = sem_unlink("/semLlamadas");
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

int borrarSemaforosBoundedBuffer (struct boundedBuffer_t *bb) {
  int error=0, status=0;
  status = sem_close(bb->lleno);
  if (!status) {
    status = sem_unlink("/lleno");
    if (!status)
      printf("Semaforo [lleno] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bb->vacio);
  if (!status) {
    status = sem_unlink("/vacio");
    if (!status)
      printf("Semaforo [vacio] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bb->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendo");
    if (!status)
      printf("Semaforo [escribiendo] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bb->leyendo);
  if (!status) {
    status = sem_unlink("/leyendo");
    if (!status)
      printf("Semaforo [leyendo] borrado!\n");
    else {
      perror("sem_unlink()");
      error -= 1;
    }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }


  return error;
}

void crearSemaforosBoundedBuffer(struct boundedBuffer_t *bb) {
  int error=0;

  bb->lleno = sem_open("/lleno", O_CREAT, 0640, 0);
  if (bb->lleno != SEM_FAILED) {
    printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->vacio = sem_open("/vacio", O_CREAT, 0640, PEDIDOS);
  if (bb->lleno != SEM_FAILED) {
    printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->leyendo = sem_open("/leyendo", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->escribiendo = sem_open("/escribiendo", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }
}

void  cargandoPedido (Encargado * encargado, int codigoPedido) {
  int error=0;
  struct boundedBuffer_t * buffer = encargado->boundedBufferPedidos;
    error = sem_wait(buffer->vacio); //pregunta si hay lugar, y en caso positvo reserva un valor
    if (!error) {
      error = sem_wait(buffer->escribiendo);//toma el semaforo escribiendo para poder escribir
    }
    if (!error) {
      //carga en el lugar correspondiente el id pedido que le pasa el telefono
      buffer->buf[buffer->fin].id = codigoPedido;
      printf("\t\tPedido %d cargado\n", buffer->buf[buffer->fin].id);
      buffer->fin = ++buffer->fin % PEDIDOS; // actualiza el proximo lugar a escribir y si exede la capacidad del buffer, vuelve al inicio
      error = sem_post(buffer->escribiendo);
    }
    if (!error) {
      error = sem_post(buffer->lleno);
      usleep(rand() % 500000);
    }
}

void  cocinarPedido(Cocinero * cocinero) {
  int error=0;
    struct boundedBuffer_t * buffer = cocinero->boundedBufferPedidos;
    error = sem_wait(buffer->lleno); // se fija si hay algun pedido por cocinar
    if (!error) {
      error = sem_wait(buffer->leyendo); // toma el semaforo leyendo para que nadie pueda leer ese dato
    }
    if (!error) {
	  printf("\t\t\tcocinando pedido %d\n", buffer->buf[buffer->inicio].id);
      if(buffer->buf[buffer->inicio].id  != -1){
          buffer->inicio = ++buffer->inicio % PEDIDOS; // actualiza el proximo lugar a leer y si exede la capacidad del buffer, vuelve al inicio
      }
      error = sem_post(buffer->leyendo);
    }
    if (!error) {
      error = sem_post(buffer->vacio);
    }
}


