#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

#define CARTA 5
#define PEDIDOS 8
#define COCINEROS 3
#define DELIVERIES 2
#define ENCARGADOS 1

enum pedidos {PIZZA, LOMITO, HAMBURGUESA, PAPAS, SANGUCHE};

//float precios[8] = {350, 200, 300, 400, 375, 275, 300, 0};

// ESTRUCTURA DEL PEDIDO
typedef struct{
  int id;
  float * precio;
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
  struct boundedBuffer_t * bbCocinero;
  struct boundedBuffer_t * bbDelivery;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
  Telefono * telefono;
  sem_t * semaforoDelivery;
  struct boundedBuffer_t * bbCocinero;
  struct boundedBuffer_t * bbCobros;
  int ultimoPedido;
}Encargado;

// ESTRUCTURA DEL DELIVERY
typedef struct{
  sem_t * semaforoDelivery;
  struct boundedBuffer_t * bbDelivery;
  struct boundedBuffer_t * bbCobros;
  int cantDeliveries;
}Delivery;

//Funciones de inicializacion de objetos
void crearSemaforosBoundedBuffer(struct boundedBuffer_t *, struct boundedBuffer_t *, struct boundedBuffer_t *);
Telefono * inicializarTelefono(sem_t *, sem_t *, Pedido *);
Encargado * inicializarEncargado(Telefono *, struct boundedBuffer_t *, struct boundedBuffer_t *, sem_t *);
Cocinero * inicializarCocinero(struct boundedBuffer_t *, struct boundedBuffer_t *);
Delivery * inicializarDelivery(sem_t *, struct boundedBuffer_t *, struct boundedBuffer_t *);

//Funciones de liberacion de memoria
void borrarSemaforo(sem_t *, sem_t *, sem_t *);
int borrarSemaforosBoundedBuffer(struct boundedBuffer_t *, struct boundedBuffer_t *);

//Funciones de telefono
void * gestionTelefono( void *);

//Funciones de encargado
void * gestionEncargado(void *);
void atenderPedido(Encargado *);
void cargandoPedido(Encargado *, int);
void cobrarPedido(Encargado *);

//Funciones de cocinero
void * gestionCocinero(void *);
void cocinarPedido(Cocinero *, int *);
void pedidoCocinado(Cocinero *);
void pedidoListo(Cocinero *, int);

//Funciones del delivery
void * gestionDelivery(void *);
void repartirPedido(Delivery *, int *);
void registrarCobro(Delivery *, int);

int main(){
  srand(time(NULL));

  // Creamos un pedido
  Pedido * pedido = (Pedido *)(calloc(1, sizeof(Pedido)));

  // Creamos un bbPedidos
  struct boundedBuffer_t * bbCocinero = (struct boundedBuffer_t *)calloc(1, sizeof(struct boundedBuffer_t));
  bbCocinero->inicio = 0;
  bbCocinero->fin    = 0;

  struct boundedBuffer_t * bbDelivery = (struct boundedBuffer_t *)calloc(1, sizeof(struct boundedBuffer_t));
  bbDelivery->inicio = 0;
  bbDelivery->fin    = 0;
  
  struct boundedBuffer_t * bbCobros = (struct boundedBuffer_t *)calloc(1, sizeof(struct boundedBuffer_t));
  bbCobros->inicio = 0;
  bbCobros->fin    = 0;

  crearSemaforosBoundedBuffer(bbCocinero, bbDelivery, bbCobros);

  // Creamos los semaforos de forma dinamica
  sem_t * semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  sem_t * semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  sem_t * semaforoDelivery = (sem_t *)(calloc(1, sizeof(sem_t)));

  semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  semaforoDelivery = sem_open("/semDelivery", O_CREAT, O_RDWR, 0);

  // Se crean los actores del juego
  Telefono * telefono = inicializarTelefono(semaforoTelefono, semaforoLlamadas, pedido);
  Encargado * encargado = inicializarEncargado(telefono, bbCocinero, bbCobros, semaforoDelivery);
  Cocinero * cocinero = inicializarCocinero(bbCocinero, bbDelivery);
  Delivery * delivery = inicializarDelivery(semaforoDelivery, bbDelivery, bbCobros);

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
  free(telefono);
  free(encargado);
  free(cocinero);
  free(delivery);
  free(pedido);
  free(bbCocinero);
  free(bbDelivery);

  borrarSemaforo(semaforoTelefono, semaforoLlamadas, semaforoDelivery);
  borrarSemaforosBoundedBuffer(bbCocinero, bbDelivery);
  return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
  Telefono *telefono = (Telefono *) tmp;
  for (int i = 0; i < PEDIDOS; i++) {
    sem_wait(telefono->semaforoTelefono);
    usleep(rand()% 100001 + 250000);
    // Le asigna un id random al pedido que luego sera un menu de la carta.
    // Si es el ultimo pedido, le pasa el valor -1 que indica finalizacion del programa.
    if(i == (PEDIDOS - 1)){
      telefono->pedido->id = -1;
      printf("\tDueño llamando para cerrar local\n");
    }
    else {
      telefono->pedido->id = i;
      printf("\ttelefono sonando\n");
    }
      //telefono->pedido->id = rand() % CARTA;
    sem_post(telefono->semaforoLlamadas);
  }
  printf("termila hilo telefono\n");
  pthread_exit(NULL);
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
  Encargado *encargado = (Encargado *) tmp;
  while(encargado->ultimoPedido != -1){
    atenderPedido(encargado);
    cobrarPedido(encargado);
  }
  printf("termila hilo encargado\n");
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

    //Si el que sigue es el ultimo pedido, repetir el mensaje para cada cocinero
    if(codigoPedido == -1){
      for (int i = 0; i < COCINEROS-1; i++)
        cargandoPedido(encargado, codigoPedido);
    }
    cargandoPedido(encargado, codigoPedido);
  }
}

void  cargandoPedido (Encargado * encargado, int codigoPedido) {
  int error=0;
  struct boundedBuffer_t * buffer = encargado->bbCocinero;
  error = sem_wait(buffer->vacio); // Pregunta si hay lugar, y en caso positvo reserva un valor
  if (!error)
    error = sem_wait(buffer->escribiendo);// Toma el semaforo escribiendo para poder escribir y que nadie mas pueda escribir.
    
  if (!error) {
    // Carga en el lugar correspondiente el id pedido que le pasa el telefono
    buffer->buf[buffer->fin].id = codigoPedido;
    usleep(rand() % 100001 + 100000);
    if( codigoPedido != -1)
      printf("\t\tPedido %d cargado\n", buffer->buf[buffer->fin].id);
    buffer->fin = ++buffer->fin % PEDIDOS; // Actualiza el proximo lugar a escribir y si exede la capacidad del buffer, vuelve al inicio
    error = sem_post(buffer->escribiendo);
  }

  if (!error)
    error = sem_post(buffer->lleno);
}

void cobrarPedido(Encargado * encargado){
  int cobrosPendientes = 0;
  sem_getvalue(encargado->semaforoDelivery, &cobrosPendientes);
  if(cobrosPendientes > 0){
    int error = 0;
    int pedidoActual = 0;
    struct boundedBuffer_t * buffer = encargado->bbCobros;
    error = sem_wait(buffer->lleno); // Se fija si hay algun pedido por cocinar
    if (!error)
      error = sem_wait(buffer->leyendo); // Toma el semaforo leyendo para que nadie pueda leer ese dato
      
    if (!error){
      pedidoActual = buffer->buf[buffer->inicio].id;
      buffer->inicio = ++buffer->inicio % DELIVERIES; // Actualiza el proximo lugar a leer y si excede la capacidad del buffer, vuelve al inicio
      error = sem_post(buffer->leyendo);
    }

    if( pedidoActual != -1) {
      usleep(rand()% 100001 + 250000);
      printf("\t\tDinero de pedido %d guardado en caja\n", pedidoActual);
      for (int i = 0; i < DELIVERIES; i++){
        printf("BUFFER ID: %d - POSICON: %d\n", buffer->buf[i].id, i);
      }
    }else{
      for (int i = 0; i < DELIVERIES; i++){
        printf("BUFFER ID: %d - POSICON: %d\n", buffer->buf[i].id, i);
      }
      encargado->ultimoPedido = -1;
    }

    if (!error)
      error = sem_post(buffer->vacio);
    
    sem_wait(encargado->semaforoDelivery);
  }
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
  int * terminado = (int *)(calloc(1, sizeof(int)));
  Cocinero *cocinero = (Cocinero *) tmp;
  //struct boundedBuffer_t * buffer = cocinero->bbCocinero;
  while(* terminado != -1)
    cocinarPedido(cocinero, terminado);
  printf("termila hilo cocinero\n");
  pthread_exit(NULL);
}

void  cocinarPedido(Cocinero * cocinero, int * terminado) {
  int error = 0;
  int pedidoActual = 0;
  struct boundedBuffer_t * buffer = cocinero->bbCocinero;
  error = sem_wait(buffer->lleno); // Se fija si hay algun pedido por cocinar
  if (!error)
    error = sem_wait(buffer->leyendo); // Toma el semaforo leyendo para que nadie pueda leer ese dato

  if (!error) {
    pedidoActual = buffer->buf[buffer->inicio].id;
    //printf("leyendo pedido %d\n", pedidoActual);
    buffer->inicio = ++buffer->inicio % PEDIDOS; // Actualiza el proximo lugar a leer y si excede la capacidad del buffer, vuelve al inicio
    error = sem_post(buffer->leyendo);
  }

  if( pedidoActual != -1) {
    printf("\t\t\tcocinando pedido %d\n", pedidoActual);
    usleep(rand()% 1000001 + 1000000);
    printf("\t\t\tpedido %d cocinado\n", pedidoActual);
    usleep(rand()% 1000001 + 1000000);
    pedidoListo(cocinero, pedidoActual);
  }else{
    cocinero->cantCocineros--;
    * terminado = -1;
  }
  if(cocinero->cantCocineros == 0){
    for(int i = 0; i < DELIVERIES; i++){
      pedidoListo(cocinero, -1);
    }
  }

  if (!error)
    error = sem_post(buffer->vacio);
}

void pedidoListo(Cocinero * cocinero, int pedidoListo){
  int error = 0;
  struct boundedBuffer_t * buffer = cocinero->bbDelivery;
  error = sem_wait(buffer->vacio); // Pregunta si hay lugar, y en caso positvo reserva un valor
  if(!error){
    error = sem_wait(buffer->escribiendo); // Toma el semaforo escribiendo para poder escribir y que nadie mas pueda escribir.
  }

  if (!error) {
    // Carga en el lugar correspondiente el nuevo pedido listo para ser repartido para el delivery.
    buffer->buf[buffer->fin].id = pedidoListo;
    usleep(rand() % 100001 + 100000);
    if( pedidoListo != -1) {
      printf("\t\t\tPedido %d listo para ser repartido\n", buffer->buf[buffer->fin].id);
    }
    else {
      for (int i = 0; i < PEDIDOS; i++){
          printf("BUFFER ID: %d - POSICON: %d\n", buffer->buf[i].id, i);
      }
    }
    buffer->fin = ++buffer->fin % PEDIDOS; // Actualiza el proximo lugar a escribir y si exede la capacidad del buffer, vuelve al inicio
    error = sem_post(buffer->escribiendo);
  }

  if (!error)
    error = sem_post(buffer->lleno);
}

// Hilo Delivery
void * gestionDelivery(void * tmp){
  Delivery * delivery = (Delivery *)(tmp);
  int * terminado = (int *)(calloc(1, sizeof(int)));
  while(*terminado != -1){
    int deliverysOcupados = 0;
    sem_getvalue(delivery->semaforoDelivery, &deliverysOcupados);
    // printf("cantidad de deliverys: %d\n",deliverysOcupados);
    if(deliverysOcupados == 0){
      repartirPedido(delivery, terminado);
    }
  }
  printf("termila hilo delivery\n");
  pthread_exit(NULL);
}

void repartirPedido(Delivery * delivery, int * terminado){
  int error = 0;
  int pedidoRepartir = 0;
  struct boundedBuffer_t * buffer = delivery->bbDelivery;
  error = sem_wait(buffer->lleno); // Se fija si hay algun pedido por repartir
  if (!error)
    error = sem_wait(buffer->leyendo); // Toma el semaforo leyendo para que nadie pueda leer ese dato

  if (!error) {
    pedidoRepartir = buffer->buf[buffer->inicio].id;
    buffer->inicio = ++buffer->inicio % PEDIDOS; // Actualiza el proximo lugar a leer y si excede la capacidad del buffer, vuelve al inicio
    printf("Proximo lugar a leer -- %d\n", buffer->inicio);
    error = sem_post(buffer->leyendo);
  }

  if( pedidoRepartir != -1) {
    printf("\t\t\t\trepartiendo pedido %d\n", pedidoRepartir);
    usleep(rand()% 1000001 + 1000000);
    printf("\t\t\t\tpedido %d entregado\n", pedidoRepartir);
    usleep(rand()% 1000001 + 1000000);
    registrarCobro(delivery, pedidoRepartir);
  }
  else {
    delivery->cantDeliveries--;
    printf("1-cantidad de deliverys que quedan: %d\n", delivery->cantDeliveries);
    * terminado = -1;
  }
  if(delivery->cantDeliveries == 0) {
    printf("2-cantidad de deliverys que quedan: %d\n",delivery->cantDeliveries);
    for(int i = 0; i < ENCARGADOS; i++) {
      registrarCobro(delivery, -1);
    }
    sem_post(delivery->semaforoDelivery);
  }

  if (!error)
    error = sem_post(buffer->vacio);
  
  if(pedidoRepartir != -1) {
    sem_post(delivery->semaforoDelivery);
  }
}

void registrarCobro(Delivery * delivery, int pedidoCobrar){
  int error=0;
  struct boundedBuffer_t * buffer = delivery->bbCobros;
  error = sem_wait(buffer->vacio); // Pregunta si hay lugar, y en caso positvo reserva un valor
  if (!error)
    error = sem_wait(buffer->escribiendo);// Toma el semaforo escribiendo para poder escribir y que nadie mas pueda escribir.
    
  if (!error) {
    // Carga en el lugar correspondiente el id pedido que le pasa el telefono
    buffer->buf[buffer->fin].id = pedidoCobrar;
    usleep(rand() % 100001 + 100000);
    if(pedidoCobrar != -1)
      printf("\t\t\t\tPedido %d listo para cobrar\n", buffer->buf[buffer->fin].id);
    buffer->fin = ++buffer->fin % DELIVERIES; // Actualiza el proximo lugar a escribir y si exede la capacidad del buffer, vuelve al inicio
    error = sem_post(buffer->escribiendo);
  }

  if (!error)
    error = sem_post(buffer->lleno);
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
Encargado * inicializarEncargado(Telefono *telefono, struct boundedBuffer_t * bbCocinero, struct boundedBuffer_t * bbCobros, sem_t * semaforoDelivery ){
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->telefono = telefono;
  encargado->bbCocinero = bbCocinero;
  encargado->bbCobros = bbCobros;
  encargado->semaforoDelivery = semaforoDelivery;
  return encargado;
}

Cocinero * inicializarCocinero(struct boundedBuffer_t * bbCocinero, struct boundedBuffer_t * bbDelivery) {
  Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
  cocinero->bbCocinero = bbCocinero;
  cocinero->bbDelivery = bbDelivery;
  cocinero->cantCocineros = COCINEROS;
  return cocinero;
}

Delivery * inicializarDelivery(sem_t * semaforoDelivery, struct boundedBuffer_t * bbDelivery, struct boundedBuffer_t * bbCobros){
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->semaforoDelivery = semaforoDelivery;
  delivery->bbDelivery = bbDelivery;
  delivery->bbCobros = bbCobros;
  delivery->cantDeliveries = DELIVERIES;
  return delivery;
}

void borrarSemaforo(sem_t * semaforoTelefono, sem_t * semaforoLlamadas, sem_t * semaforoDelivery){
  int error = 0;
  error = sem_close(semaforoTelefono);
  if(!error){
      error = sem_unlink("/semTelefono");
      if(error){
          perror("sem_unlink()");
      }
      // else {
      //     printf("Semaforo borrado!\n");
      // }
  }else{
      perror("sem_close()");
  }

  error = sem_close(semaforoLlamadas);
  if(!error){
      error = sem_unlink("/semLlamadas");
      if(error){
          perror("sem_unlink()");
      }
      // else {
      //     printf("Semaforo borrado!\n");
      // }        
  }else{
      perror("sem_close()");
  }

  error = sem_close(semaforoDelivery);
  if(!error){
      error = sem_unlink("/semDelivery");
      if(error){
          perror("sem_unlink()");
      }
      // else {
      //     printf("Semaforo borrado!\n");
      // }        
  }else{
      perror("sem_close()");
  }
}

int borrarSemaforosBoundedBuffer (struct boundedBuffer_t *bbCocinero, struct boundedBuffer_t *bbDelivery) {
  int error=0, status=0;
  status = sem_close(bbCocinero->lleno);
  if (!status) {
    status = sem_unlink("/llenoCocinero");
    // if (!status)
    //   printf("Semaforo [lleno] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbCocinero->vacio);
  if (!status) {
    status = sem_unlink("/vacioCocinero");
    // if (!status)
    //   printf("Semaforo [vacio] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbCocinero->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendoCocinero");
    // if (!status)
    //   printf("Semaforo [escribiendo] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbCocinero->leyendo);
  if (!status) {
     status = sem_unlink("/leyendoCocinero");
    // if (!status)
    //   printf("Semaforo [leyendo] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbDelivery->lleno);
  if (!status) {
    status = sem_unlink("/llenoDelivery");
    // if (!status)
    //   printf("Semaforo [lleno] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbDelivery->vacio);
  if (!status) {
    status = sem_unlink("/vacioDelivery");
    // if (!status)
    //   printf("Semaforo [vacio] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbDelivery->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendoDelivery");
    // if (!status)
    //   printf("Semaforo [escribiendo] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }

  status = sem_close(bbDelivery->leyendo);
  if (!status) {
    status = sem_unlink("/leyendoDelivery");
    // if (!status)
    //   printf("Semaforo [leyendo] borrado!\n");
    // else {
    //   perror("sem_unlink()");
    //   error -= 1;
    // }
  }
  else {
    perror("sem_close()");
    error -= 1;
  }
  
  return error;
}

void crearSemaforosBoundedBuffer(struct boundedBuffer_t *bbCocinero, struct boundedBuffer_t *bbDelivery, struct boundedBuffer_t *bbCobros) {
  int error=0;

  bbCocinero->lleno = sem_open("/llenoCocinero", O_CREAT, 0640, 0);
  if (bbCocinero->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCocinero->vacio = sem_open("/vacioCocinero", O_CREAT, 0640, PEDIDOS);
  if (bbCocinero->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCocinero->leyendo = sem_open("/leyendoCocinero", O_CREAT, 0640, 1);
  if (bbCocinero->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCocinero->escribiendo = sem_open("/escribiendoCocinero", O_CREAT, 0640, 1);
  if (bbCocinero->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbDelivery->lleno = sem_open("/llenoDelivery", O_CREAT, 0640, 0);
  if (bbDelivery->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbDelivery->vacio = sem_open("/vacioDelivery", O_CREAT, 0640, PEDIDOS);
  if (bbDelivery->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbDelivery->leyendo = sem_open("/leyendoDelivery", O_CREAT, 0640, 1);
  if (bbDelivery->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbDelivery->escribiendo = sem_open("/escribiendoDelivery", O_CREAT, 0640, 1);
  if (bbDelivery->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

   bbCobros->lleno = sem_open("/llenoCobros", O_CREAT, 0640, 0);
  if (bbCobros->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCobros->vacio = sem_open("/vacioCobros", O_CREAT, 0640, DELIVERIES);
  if (bbCobros->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCobros->leyendo = sem_open("/leyendoCobros", O_CREAT, 0640, 1);
  if (bbCobros->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bbCobros->escribiendo = sem_open("/escribiendoCobros", O_CREAT, 0640, 1);
  if (bbCobros->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }
}