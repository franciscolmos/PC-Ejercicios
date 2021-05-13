#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */
#include <semaphore.h>
#include <fcntl.h>

#include "MonitoresBuffer.h"

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
float precios[CARTA] = {250, 350, 300, 150, 250};

// ESTRUCTURA DEL PEDIDO
typedef struct{
  int id;
  float precio;
}Pedido;

// ESTRUCTURA DEL TELEFONO
typedef struct{
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  Pedido * pedido;
}Telefono;

// ESTRUCTURA DEL COCINERO
typedef struct{
  struct Monitor_t *monitorComandas;
  struct Monitor_t *monitorPedidos;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
  Telefono * telefono;
  Pedido * pedidoPorCobrar;
  struct Monitor_t *monitorComandas;
  sem_t * semaforoPedidosPorCobrar; // verificando constantemente este valor y cuando es > 0
  sem_t * semaforoDejarDinero; // hace post sobre este semaforo para dejar pasar al primer delivery que esta listo para entregar el dinero
  sem_t * semaforoTomarDinero; // printea dinero en caja y hace wait para indicar esto. Luego haca wait a semaforoPedidosPorCobrar
  // Podriamos hacer que el encargado termine igual que los cocineros y el delivery
  // ya que, cuando tengamos mas de un encargado, esta variable nos va a causar problemas.
  int ultimoPedido;
}Encargado;

// ESTRUCTURA DEL DELIVERY
typedef struct{
  sem_t * semaforoPedidosPorCobrar; // el delivery hace post cuando vuelve a la pizzeria
  sem_t * semaforoDejarDinero; // y automaticamanete hace wait sobre dejarDinero y se encola
  sem_t * semaforoTomarDinero; // printea entregadno dinero encargado  y luego hace post en tomarDinero
  struct Monitor_t *monitorPedidos;
  int cantDeliveries;
  Pedido * pedidoPorCobrar;
}Delivery;

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Actores
Telefono * crearTelefono();
Encargado * crearEncargado(Telefono *, struct Monitor_t *, sem_t *, sem_t *, sem_t *, Pedido *);
Cocinero * crearCocinero(struct Monitor_t *, struct Monitor_t *);
Delivery * crearDelivery(sem_t *, sem_t *, sem_t *, struct Monitor_t *, Pedido *);

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

  // Creamos los monitores
  struct Monitor_t *monitorComandas = NULL;
  monitorComandas = CrearMonitor(4);
  struct Monitor_t *monitorPedidos = NULL;
  monitorPedidos = CrearMonitor(4);

  srand(time(NULL));

  sem_t * semaforoPedidosPorCobrar = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoPedidosPorCobrar = sem_open("/semPedidosPorCobrar", O_CREAT, O_RDWR, 0);

  sem_t * semaforoDejarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoDejarDinero = sem_open("/semDejarDinero", O_CREAT, O_RDWR, 0);

  sem_t * semaforoCobrarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoCobrarDinero = sem_open("/semCobrarDinero", O_CREAT, O_RDWR, 0);

  Pedido * pedidoPorCobrar = (Pedido *)(calloc(1, sizeof(Pedido)));

  // Se crean los actores del juego
  Telefono * telefono = crearTelefono();
  Encargado * encargado = crearEncargado(telefono, monitorComandas, semaforoPedidosPorCobrar, semaforoDejarDinero, semaforoCobrarDinero, pedidoPorCobrar);
  Cocinero * cocinero = crearCocinero(monitorComandas, monitorPedidos);
  Delivery * delivery = crearDelivery(semaforoPedidosPorCobrar, semaforoDejarDinero, semaforoCobrarDinero, monitorPedidos, pedidoPorCobrar);

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

  // Borramos los monitores
  BorrarMonitor(monitorComandas);
  BorrarMonitor(monitorPedidos);

  // Se libera la memoria de los objetos creados
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

  usleep(rand() % 100001 + 100000);
  if( codigoPedido != -1)
    printf("\t\tPedido %d cargado\n", codigoPedido);

  error = GuardarDato(encargado->monitorComandas, codigoPedido);
  if(error)
    perror("GuardarDato()");
}

void cobrarPedido(Encargado * encargado){
  int cobrosPendientes = 0;
  // Se fija si hay algun delivery esperando para que le cobre
  sem_getvalue(encargado->semaforoPedidosPorCobrar, &cobrosPendientes);
  if(cobrosPendientes > 0){
    sem_post(encargado->semaforoDejarDinero);
    sem_wait(encargado->semaforoTomarDinero);
    if( encargado->pedidoPorCobrar->id != -1) {
      printf("\t\t$%.0f guardado de pedido %d\n", encargado->pedidoPorCobrar->precio, encargado->pedidoPorCobrar->id);
    }
    else {
      printf("\t\tCerrando local\n");
      encargado->ultimoPedido = -1;
    }
    sem_post(encargado->semaforoPedidosPorCobrar);
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
  error = LeerDato(cocinero->monitorComandas, &pedidoActual);
  if(error)
    perror("LeerDato()");
  else {
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
  }
}

void pedidoListo(Cocinero * cocinero, int pedidoListo){
  int error = 0;

  usleep(rand() % 100001 + 250000);
  if( pedidoListo != -1)
    printf("\t\t\tPedido %d listo para ser repartido\n", pedidoListo);
    
  error = GuardarDato(cocinero->monitorPedidos, pedidoListo);
  if(error)
    perror("GuardarDato()");
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
  error = LeerDato(delivery->monitorPedidos, &pedidoRepartir);
  if(error)
    perror("LeerDato()");
  else {
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
  }
}

void avisarCobro(Delivery * delivery, int pedidoCobrar){
  sem_post(delivery->semaforoPedidosPorCobrar);
  sem_wait(delivery->semaforoDejarDinero);
  if(pedidoCobrar != -1) {
    printf("\t\t\t\tdejando dinero de pedido %d\n", pedidoCobrar);
  }
  usleep(100000);
  delivery->pedidoPorCobrar->id = pedidoCobrar;
  delivery->pedidoPorCobrar->precio = precios[rand()%CARTA];
  sem_post(delivery->semaforoTomarDinero);
}

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
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
Encargado * crearEncargado(Telefono *telefono, struct Monitor_t * monitorComandas, sem_t * semaforoPedidosPorCobrar, sem_t * semaforoDejarDinero, sem_t * semaforoTomarDinero, Pedido * pedidoPorCobrar){
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->telefono = telefono;
  encargado->monitorComandas = monitorComandas;
  encargado->semaforoPedidosPorCobrar = semaforoPedidosPorCobrar;
  encargado->semaforoDejarDinero = semaforoDejarDinero;
  encargado->semaforoTomarDinero = semaforoTomarDinero;
  encargado->pedidoPorCobrar = pedidoPorCobrar;
  return encargado;
}

// Creacion de Cocinero
Cocinero * crearCocinero(struct Monitor_t * monitorComandas, struct Monitor_t * monitorPedidos) {
  Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
  cocinero->monitorComandas = monitorComandas;
  cocinero->monitorPedidos = monitorPedidos;
  cocinero->cantCocineros = COCINEROS;
  return cocinero;
}

// Creacion de Delivery
Delivery * crearDelivery(sem_t * semaforoPedidosPorCobrar, sem_t * semaforoDejarDinero, sem_t * semaforoTomarDinero, struct Monitor_t * monitorPedidos, Pedido * pedidoPorCobrar){
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->semaforoPedidosPorCobrar = semaforoPedidosPorCobrar;
  delivery->semaforoDejarDinero = semaforoDejarDinero;
  delivery->semaforoTomarDinero = semaforoTomarDinero;
  delivery->monitorPedidos = monitorPedidos;
  delivery->pedidoPorCobrar = pedidoPorCobrar;
  delivery->cantDeliveries = DELIVERIES;
  return delivery;
}

/*-----------------FUNCIONES DE LIBERACION DE MEMORIA---------------------*/


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

  // Semaforo PedidosPorCobrar
  status = sem_close(enc->semaforoPedidosPorCobrar);
  if (!status) {
    status = sem_unlink("/semPedidosPorCobrar");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo DejarDinero
  status = sem_close(enc->semaforoDejarDinero);
  if (!status) {
    status = sem_unlink("/semDejarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo TomarDinero
  status = sem_close(enc->semaforoTomarDinero);
  if (!status) {
    status = sem_unlink("/semCobrarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");
}
