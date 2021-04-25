#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

// Cantidad de actores
#define ENCARGADOS 1
#define COCINEROS 3
#define DELIVERIES 2

// Datos de entrada
#define CARTA 5
#define PEDIDOS 8

// Carta ofrecida por la pizzeria
//enum pedidos {PIZZA, LOMITO, HAMBURGUESA, PAPAS, SANGUCHE};

// Precios de los menues
//float precios[8] = {250, 350, 300, 150, 250};

// ESTRUCTURA DEL PEDIDO
typedef struct{
  int id;
  float * precio;
}Pedido;

// BUFFER CIRUCULAR PARA ALMACENAR PEDIDOS
typedef struct{
  int inicio, fin; // fin-->proximo valor libre a escribir. Inicio-->proximo dato a leer
  Pedido buf[PEDIDOS];
  sem_t *lleno, *vacio, *leyendo, *escribiendo;
}bBuffer;

// ESTRUCTURA DEL TELEFONO
typedef struct{
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  Pedido * pedido;
}Telefono;

// ESTRUCTURA DEL COCINERO
typedef struct{
  bBuffer * bbCocinero;
  bBuffer * bbDelivery;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
  Telefono * telefono;
  sem_t * semaforoDelivery;
  bBuffer * bbCocinero;
  bBuffer * bbCobros;
  // Podriamos hacer que el encargado termine igual que los cocineros y el delivery
  // ya que, cuando tengamos mas de un encargado, esta variable nos va a causar problemas.
  int ultimoPedido;
}Encargado;

// ESTRUCTURA DEL DELIVERY
typedef struct{
  sem_t * semaforoDelivery;
  bBuffer * bbDelivery;
  bBuffer * bbCobros;
  int cantDeliveries;
}Delivery;

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Buffers
bBuffer * crearBufferCocinero();
bBuffer * crearBufferDelivery();
bBuffer * crearBufferCobros();

// Actores
Telefono * crearTelefono();
Encargado * crearEncargado(Telefono *, bBuffer *, bBuffer *, sem_t *);
Cocinero * crearCocinero(bBuffer *, bBuffer *);
Delivery * crearDelivery(sem_t *, bBuffer *, bBuffer *);

/*-------------------FUNCIONES DE LIBREACION DE MEMORIA-----------------------*/
// Borrado de semaforos
void borrarSemaforos(Encargado *, Delivery *);

/*---------------------FUNCIONES DE ACTORES DEL JUEGO-------------------------*/
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
void avisarCobro(Delivery *, int);

/*-----------------------------------------------------------------------------*/
/*----------------------------------MAIN---------------------------------------*/

int main(){
  srand(time(NULL));

  // Creamos los buffers circulares que seran compartidos entre los actores
  bBuffer * bbCocinero = crearBufferCocinero();
  bBuffer * bbDelivery = crearBufferDelivery();
  bBuffer * bbCobros = crearBufferCobros();

  // Creamos el semaforo delivery que sera compartido entre Delivery y Encargado
  sem_t * semaforoDelivery = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoDelivery = sem_open("/semDelivery", O_CREAT, O_RDWR, 0);

  // Se crean los actores del juego
  Telefono * telefono = crearTelefono();
  Encargado * encargado = crearEncargado(telefono, bbCocinero, bbCobros, semaforoDelivery);
  Cocinero * cocinero = crearCocinero(bbCocinero, bbDelivery);
  Delivery * delivery = crearDelivery(semaforoDelivery, bbDelivery, bbCobros);

  // Se instancian las variables hilos de cada objeto
  pthread_t hiloTelefono;
  pthread_t hiloEncargado;
  pthread_t hilosCocineros[COCINEROS];
  pthread_t hilosDeliveries[DELIVERIES];

  // Se crean los hilos de cada objeto
  pthread_create(&hiloTelefono, NULL, gestionTelefono, (void *)(telefono));
  pthread_create(&hiloEncargado, NULL, gestionEncargado, (void *)(encargado));
  for(int i = 0; i < COCINEROS; i++) {
    pthread_create(&hilosCocineros[i], NULL, gestionCocinero, (void *)(cocinero));
  }
  for(int i = 0; i < DELIVERIES; i++) {
    pthread_create(&hilosDeliveries[i], NULL, gestionDelivery, (void *)(delivery));
  }

  // Se espera que terminen todos los hilos
  pthread_join(hiloTelefono, NULL);
  pthread_join(hiloEncargado, NULL);
  for(int i = 0; i < COCINEROS; i++) {
    pthread_join(hilosCocineros[i], NULL);
  }
  for(int i = 0; i < DELIVERIES; i++) {
    pthread_join(hilosDeliveries[i], NULL);
  }

  // Se liberan los semaforos
  borrarSemaforos(encargado, delivery);

  // Se libera la memoria de los objetos creados
  free(bbCocinero);
  free(bbDelivery);
  free(bbCobros);
  free(telefono);
  free(encargado);
  free(cocinero);
  free(delivery);

  return 0;
}

// Hilo telefono
void * gestionTelefono(void * tmp){
  Telefono *telefono = (Telefono *) tmp;
  for (int i = 0; i < PEDIDOS; i++) {
    sem_wait(telefono->semaforoTelefono);
    usleep(rand()% 750001 + 250000);
    // Si es el ultimo pedido, le pasa el valor -1 que indica finalizacion del programa.
    if(i == (PEDIDOS - 1)){
      telefono->pedido->id = -1;
      printf("\tDueño llamando para cerrar local\n");
    }
    else {
      telefono->pedido->id = i;
      printf("\ttelefono sonando\n");
    }
    sem_post(telefono->semaforoLlamadas);
  }
  pthread_exit(NULL);
}

// Hilo Encargado
void * gestionEncargado(void * tmp){
  Encargado *encargado = (Encargado *) tmp;
  while(encargado->ultimoPedido != -1){
    atenderPedido(encargado);
    cobrarPedido(encargado);
  }
  // int semaforo = 0;
  // sem_getvalue(encargado->telefono->semaforoTelefono, &semaforo);
  // printf("El valor del semaforo telefono es: %d\n", semaforo);
  // sem_getvalue(encargado->telefono->semaforoLlamadas, &semaforo);
  // printf("El valor del semaforo llamadas es: %d\n", semaforo);
  // sem_getvalue(encargado->semaforoDelivery, &semaforo);
  // printf("El valor del semaforo deliveries es: %d\n", semaforo);
  pthread_exit(NULL);
}

void atenderPedido(Encargado * encargado){
  int error = 0;
  // Verifica si hay alguna llamada entrante
  error = sem_trywait(encargado->telefono->semaforoLlamadas);
  if(!error){
    printf("\t\ttelefono atendido\n");
    usleep(rand()% 100001 + 250000);
    int codigoPedido = encargado->telefono->pedido->id;
    printf("\t\ttelefono colgado\n");
    sem_post(encargado->telefono->semaforoTelefono);

    //Si el que sigue es el ultimo pedido, avisar a cada cocinero para que terminen
    if(codigoPedido == -1){
      for (int i = 0; i < COCINEROS-1; i++)
        cargandoPedido(encargado, codigoPedido);
    }
    cargandoPedido(encargado, codigoPedido);
  }
}

void cargandoPedido (Encargado * encargado, int codigoPedido) {
  int error=0;
  bBuffer * buffer = encargado->bbCocinero;
  error = sem_wait(buffer->vacio);
  if (!error)
    error = sem_wait(buffer->escribiendo);
    
  if (!error) {
    // Carga en el lugar correspondiente el id pedido que le pasa el telefono
    buffer->buf[buffer->fin].id = codigoPedido;
    usleep(rand() % 100001 + 100000);
    if( codigoPedido != -1)
      printf("\t\tPedido %d cargado\n", codigoPedido);
    buffer->fin = ++buffer->fin % PEDIDOS;
    error = sem_post(buffer->escribiendo);
  }

  if (!error)
    error = sem_post(buffer->lleno);
}

void cobrarPedido(Encargado * encargado){
  int cobrosPendientes = 0;
  // Se fija si hay algun delivery esperando para que le cobre
  sem_getvalue(encargado->semaforoDelivery, &cobrosPendientes);
  if(cobrosPendientes > 0){
    int error = 0;
    int pedidoActual = 0;
    bBuffer * buffer = encargado->bbCobros;
    error = sem_wait(buffer->lleno);
    if (!error)
      error = sem_wait(buffer->leyendo);
      
    if (!error){
      pedidoActual = buffer->buf[buffer->inicio].id;
      buffer->inicio = ++buffer->inicio % DELIVERIES;
      error = sem_post(buffer->leyendo);
    }

    // Si el pedido actual es -1, significa que tiene que cerrar el local.
    if( pedidoActual != -1) {
      usleep(rand()% 100001 + 250000);
      printf("\t\tDinero de pedido %d guardado en caja\n", pedidoActual);
    }
    else {
      encargado->ultimoPedido = -1;
    }

    if (!error) {
      error = sem_post(buffer->vacio);
    }
    
    // Libera al delivery para que entregue otro pedido
    sem_wait(encargado->semaforoDelivery);
  }
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
  int * terminado = (int *)(calloc(1, sizeof(int)));
  Cocinero *cocinero = (Cocinero *) tmp;
  while(* terminado != -1) {
    cocinarPedido(cocinero, terminado);
  }
  pthread_exit(NULL);
}

void  cocinarPedido(Cocinero * cocinero, int * terminado) {
  int error = 0;
  int pedidoActual = 0;
  bBuffer * buffer = cocinero->bbCocinero;
  error = sem_wait(buffer->lleno);
  if (!error)
    error = sem_wait(buffer->leyendo);

  if (!error) {
    pedidoActual = buffer->buf[buffer->inicio].id;
    buffer->inicio = ++buffer->inicio % PEDIDOS;
    error = sem_post(buffer->leyendo);
  }

  // Si el pedido actual es -1, entonces empieza a cerrar la cocina
  if( pedidoActual != -1) {
    printf("\t\t\tcocinando pedido %d\n", pedidoActual);
    usleep(rand()% 500001 + 1000000);
    printf("\t\t\tpedido %d cocinado\n", pedidoActual);
    pedidoListo(cocinero, pedidoActual);
  }
  else {
    cocinero->cantCocineros--;
    * terminado = -1;
  }

  // El ultimo cocinero es el encargado de avisar a los deliveries que ya cerró la cocina
  if(cocinero->cantCocineros == 0) {
    for(int i = 0; i < DELIVERIES; i++) {
      pedidoListo(cocinero, -1);
    }
  }

  if (!error)
    error = sem_post(buffer->vacio);
}

void pedidoListo(Cocinero * cocinero, int pedidoListo){
  int error = 0;
  bBuffer * buffer = cocinero->bbDelivery;
  error = sem_wait(buffer->vacio);
  if(!error) {
    error = sem_wait(buffer->escribiendo);
  }

  if (!error) {
    buffer->buf[buffer->fin].id = pedidoListo;
    usleep(rand() % 100001 + 250000);
    if( pedidoListo != -1) {
      printf("\t\t\tPedido %d listo para ser repartido\n", pedidoListo);
    }
    buffer->fin = ++buffer->fin % PEDIDOS;
    error = sem_post(buffer->escribiendo);
  }

  if (!error) {
    error = sem_post(buffer->lleno);
  }
}

// Hilo Delivery
void * gestionDelivery(void * tmp){
  Delivery * delivery = (Delivery *)(tmp);
  int * terminado = (int *)(calloc(1, sizeof(int)));
  while(*terminado != -1) {
    repartirPedido(delivery, terminado);
  }
  pthread_exit(NULL);
}

void repartirPedido(Delivery * delivery, int * terminado) {
  int error = 0;
  int pedidoRepartir = 0;
  bBuffer * buffer = delivery->bbDelivery;
  error = sem_wait(buffer->lleno);
  if (!error) {
    int deliverysOcupados = 1;
    while( deliverysOcupados > 0) {
      sem_getvalue(delivery->semaforoDelivery, &deliverysOcupados);
    }
    // Si no hay deliveries ocupados (esperando a darle plata al encargado)
    // entonces puede(n) salir a repartir
      error = sem_wait(buffer->leyendo);
  }

  if (!error) {
    pedidoRepartir = buffer->buf[buffer->inicio].id;
    buffer->inicio = ++buffer->inicio % PEDIDOS;
    error = sem_post(buffer->leyendo);
  }

  // Si el pedido a repartir es -1, entonces termina su trabajo
  if( pedidoRepartir != -1) {
    printf("\t\t\t\trepartiendo pedido %d\n", pedidoRepartir);
    usleep(rand()% 250001 + 350000);
    printf("\t\t\t\tpedido %d entregado\n", pedidoRepartir);
    usleep(rand()% 250001 + 350000);
    avisarCobro(delivery, pedidoRepartir);
  }
  else {
    delivery->cantDeliveries--;
    * terminado = -1;
  }

  // Si es el ultimo delivery que termina, avisa al encargado que termino
  if(delivery->cantDeliveries == 0) {
    for(int i = 0; i < ENCARGADOS; i++) {
      avisarCobro(delivery, -1);
    }
  }

  if (!error)
    error = sem_post(buffer->vacio);
}

void avisarCobro(Delivery * delivery, int pedidoCobrar){
  int error=0;
  bBuffer * buffer = delivery->bbCobros;
  error = sem_wait(buffer->vacio);
  if (!error)
    error = sem_wait(buffer->escribiendo);
    
  if (!error) {
    buffer->buf[buffer->fin].id = pedidoCobrar;
    if(pedidoCobrar != -1) {
      printf("\t\t\t\tPedido %d listo para cobrar\n", pedidoCobrar);
    }
    sem_post(delivery->semaforoDelivery);
    buffer->fin = ++buffer->fin % DELIVERIES;
    error = sem_post(buffer->escribiendo);
  }

  if (!error)
    error = sem_post(buffer->lleno);
}

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/

// Creacion de Buffer de Cocinero
bBuffer * crearBufferCocinero() {
  int error = 0;

  bBuffer * bb = (bBuffer *)calloc(1, sizeof(bBuffer));

  bb->lleno = sem_open("/llenoCocinero", O_CREAT, 0640, 0);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->vacio = sem_open("/vacioCocinero", O_CREAT, 0640, DELIVERIES);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->leyendo = sem_open("/leyendoCocinero", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->escribiendo = sem_open("/escribiendoCocinero", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  return bb;
}

// Creacion de Buffer de Delivery
bBuffer * crearBufferDelivery() {
  int error = 0;

  bBuffer * bb = (bBuffer *)calloc(1, sizeof(bBuffer));

  bb->lleno = sem_open("/llenoDelivery", O_CREAT, 0640, 0);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->vacio = sem_open("/vacioDelivery", O_CREAT, 0640, DELIVERIES);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->leyendo = sem_open("/leyendoDelivery", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->escribiendo = sem_open("/escribiendoDelivery", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  return bb;
}

// Creacion de Buffer de Cobros
bBuffer * crearBufferCobros() {

  int error = 0;

  bBuffer * bb = (bBuffer *)calloc(1, sizeof(bBuffer));

  bb->lleno = sem_open("/llenoCobros", O_CREAT, 0640, 0);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [lleno] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->vacio = sem_open("/vacioCobros", O_CREAT, 0640, DELIVERIES);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [vacio] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->leyendo = sem_open("/leyendoCobros", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [leyendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  bb->escribiendo = sem_open("/escribiendoCobros", O_CREAT, 0640, 1);
  if (bb->lleno != SEM_FAILED) {
    // printf("Semaforo [escribiendo] creado!\n");
  }
  else {
    perror("sem_open()");
    error -= 1;
  }

  return bb;
}

// Creacion de Telefono
Telefono * crearTelefono() {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  telefono->semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  telefono-> pedido = (Pedido *)(calloc(1, sizeof(Pedido)));;
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(Telefono *telefono, bBuffer * bbCocinero, bBuffer * bbCobros, sem_t * semaforoDelivery ){
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->telefono = telefono;
  encargado->bbCocinero = bbCocinero;
  encargado->bbCobros = bbCobros;
  encargado->semaforoDelivery = semaforoDelivery;
  return encargado;
}

// Creacion de Cocinero
Cocinero * crearCocinero(bBuffer * bbCocinero, bBuffer * bbDelivery) {
  Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
  cocinero->bbCocinero = bbCocinero;
  cocinero->bbDelivery = bbDelivery;
  cocinero->cantCocineros = COCINEROS;
  return cocinero;
}

// Creacion de Delivery
Delivery * crearDelivery(sem_t * semaforoDelivery, bBuffer * bbDelivery, bBuffer * bbCobros){
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->semaforoDelivery = semaforoDelivery;
  delivery->bbDelivery = bbDelivery;
  delivery->bbCobros = bbCobros;
  delivery->cantDeliveries = DELIVERIES;
  return delivery;
}

// Borrado de semaforos
void borrarSemaforos(Encargado * enc, Delivery * del) {
  int status=0;
  // Semaforo Telefono
  status = sem_close(enc->telefono->semaforoTelefono);
  if (!status) {
    status = sem_unlink("/semTelefono");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Llamadas
  status = sem_close(enc->telefono->semaforoLlamadas);
  if (!status) {
    status = sem_unlink("/semLlamadas");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Delivery
  status = sem_close(enc->semaforoDelivery);
  if (!status) {
    status = sem_unlink("/semDelivery");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cocinero Lleno
  status = sem_close(enc->bbCocinero->lleno);
  if (!status) {
    status = sem_unlink("/llenoCocinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cocinero Vacio
  status = sem_close(enc->bbCocinero->vacio);
  if (!status) {
    status = sem_unlink("/vacioCocinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cocinero Escribiendo
  status = sem_close(enc->bbCocinero->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendoCocinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cocinero Leyendo
  status = sem_close(enc->bbCocinero->leyendo);
  if (!status) {
    status = sem_unlink("/leyendoCocinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");
  
  // Semaforo Buffer Delivery Lleno
  status = sem_close(del->bbDelivery->lleno);
  if (!status) {
    status = sem_unlink("/llenoDelivery");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Delivery Vacio
  status = sem_close(del->bbDelivery->vacio);
  if (!status) {
    status = sem_unlink("/vacioDelivery");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Delivery Escribiendo
  status = sem_close(del->bbDelivery->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendoDelivery");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Delivery Leyendo
  status = sem_close(del->bbDelivery->leyendo);
  if (!status) {
    status = sem_unlink("/leyendoDelivery");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cobros Lleno
  status = sem_close(enc->bbCobros->lleno);
  if (!status) {
    status = sem_unlink("/llenoCobros");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cobros Vacio
  status = sem_close(enc->bbCobros->vacio);
  if (!status) {
    status = sem_unlink("/vacioCobros");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cobros Escribiendo
  status = sem_close(enc->bbCobros->escribiendo);
  if (!status) {
    status = sem_unlink("/escribiendoCobros");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo Buffer Cobros Leyendo
  status = sem_close(enc->bbCobros->leyendo);
  if (!status) {
    status = sem_unlink("/leyendoCobros");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");
}
