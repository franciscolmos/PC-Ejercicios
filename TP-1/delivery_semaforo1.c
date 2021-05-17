#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "MonitoresBuffer.h"

// Cantidad de actores
#define ENCARGADOS 1
#define COCINEROS 3
#define DELIVERIES 2

// Datos de entrada
#define CARTA 5
#define ALARMA 5

// Dato que indica que el juego termino
int timeout = 1;

// Precios de la carta
float precios[CARTA] = {250, 350, 300, 150, 250};

/*--------------------------------ESTRUCTURAS-------------------------------*/
// ESTRUCTURA DE LA MEMORIA
typedef struct {
  sem_t * semaforoPedidosPorCobrar; //Delivery: el delivery hace post cuando vuelve a la pizzeria | Encargado: verificando constantemente este valor y cuando es > 0
  sem_t * semaforoDejarDinero; // Delivery: y automaticamanete hace wait sobre dejarDinero y se encola | Encargado: hace post sobre este semaforo para dejar pasar al primer delivery que esta listo para entregar el dinero.
  sem_t * semaforoCobrarDinero; // Delivery: printea entregadno dinero encargado  y luego hace post en tomarDinero | Encargado: printea dinero en caja y hace wait para indicar esto. Luego haca wait a semaforoPedidosPorCobrar.
  int dato;
}Memoria;

// ESTRUCTURA DEL TELEFONO
typedef struct{
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  int pedido;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
  Telefono * telefono;
  struct Monitor_t *monitorComandas;
  int ultimoPedido;
  int ubiMemoria;
  Memoria * memoria;
}Encargado;

// ESTRUCTURA DEL COCINERO
typedef struct{
  struct Monitor_t *monitorComandas;
  struct Monitor_t *monitorPedidos;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL DELIVERY
typedef struct{
  struct Monitor_t *monitorPedidos;
  int cantDeliveries;
  int ubiMemoria;
  Memoria * memoria;
}Delivery;

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Actores
Telefono * crearTelefono();
Encargado * crearEncargado(Telefono *, struct Monitor_t *, int);
Cocinero * crearCocinero(struct Monitor_t *, struct Monitor_t *);
Delivery * crearDelivery(struct Monitor_t *, int);

// Memoria
int crearMemoria();
void llenarMemoria(int);

/*-------------------------FUNCIONES DEL JUGADOR------------------------------*/
void menu(Encargado *);
void mostrarMenu();

/*---------------------FUNCIONES DE ACTORES DEL JUEGO-------------------------*/
//Funciones de telefono
void * gestionTelefono( void *);
void TimeOut();

//Funciones de encargado
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


/*-------------------FUNCIONES DE LIBREACION DE MEMORIA-----------------------*/
// Borrado de semaforos y memoria
void borrarSemMem(Encargado *);

/*-----------------------------------------------------------------------------*/
/*----------------------------------MAIN---------------------------------------*/

int main(){
  srand(time(NULL));

  // Creamos los monitores
  struct Monitor_t * monitorComandas = CrearMonitor(4);
  struct Monitor_t * monitorPedidos = CrearMonitor(4);

  // Creamos la memoria y traemos su ubiacion
  int memoria =  crearMemoria();

  // Se crean los actores del juego
  Telefono * telefono = crearTelefono();
  Encargado * encargado = crearEncargado(telefono, monitorComandas, memoria);
  Cocinero * cocinero = crearCocinero(monitorComandas, monitorPedidos);
  Delivery * delivery = crearDelivery(monitorPedidos, memoria);

  // Se instancian las variables hilos de cada objeto
  pthread_t hiloTelefono;
  pthread_t hilosCocineros[COCINEROS];
  pthread_t hilosDeliveries[DELIVERIES];

  // Se crean los hilos de cada objeto
  pthread_create(&hiloTelefono, NULL, gestionTelefono, (void *)(telefono));
  for(int i = 0; i < COCINEROS; i++) {
    pthread_create(&hilosCocineros[i], NULL, gestionCocinero, (void *)(cocinero));
  }
  for(int i = 0; i < DELIVERIES; i++) {
    pthread_create(&hilosDeliveries[i], NULL, gestionDelivery, (void *)(delivery));
  }

  // Gestion Encargado
  encargado->memoria = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, memoria, 0);
  while(encargado->ultimoPedido != -1){
    atenderPedido(encargado);
    cobrarPedido(encargado);
  }

  // HACE FALTA ESPERAR QUE TERMINEN LOS HILOS? SE SUPONE QUE EL
  // ENCARGADO TERMINA CUANDO EL ULTIMO DELIVERY LE AVISA QUE TERMINO
  // Se espera que terminen todos los hilos
  pthread_join(hiloTelefono, NULL);
  for(int i = 0; i < COCINEROS; i++) {
    pthread_join(hilosCocineros[i], NULL);
  }
  for(int i = 0; i < DELIVERIES; i++) {
    pthread_join(hilosDeliveries[i], NULL);
  }

  // Se liberan los semaforos
  borrarSemMem(encargado);

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

// Menu
void menu(Encargado * encargado) {
  encargado->memoria = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, encargado->ubiMemoria, 0);
  int terminar = 0;
  char eleccion;
  do
  {
    mostrarMenu();
    scanf(eleccion);
  } while (terminar);
  
  // while(encargado->ultimoPedido != -1){
  //   atenderPedido(encargado);
  //   cobrarPedido(encargado);
  // }
}

void mostrarMenu() {
  char temp;
  printf("|-------------------------------------------------------------------|");
  printf("|--------------------     PIZZERIA      ----------------------------|");
  printf("|                                                                   |");
  printf("| 1. Comenzar juego.                                                |");
  printf("| 2. Ver puntuacion.                                                |");
  printf("| 3. Salir.                                                         |");
  printf("|-------------------------------------------------------------------|");
  printf("Ingrese una opcion: ");
  scanf(temp);
}

// Hilo telefono
void * gestionTelefono(void * tmp){
  Telefono *telefono = (Telefono *) tmp;

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);
  int terminar = 1;
  while(terminar) {
    sem_wait(telefono->semaforoTelefono);
    usleep(rand()% 750001 + 250000);

    // Si es el ultimo pedido, le pasa el valor -1 que indica finalizacion del programa.
    if(timeout){
      telefono->pedido = rand() % CARTA;
      printf("\ttelefono sonando\n");
    }
    else {
      terminar = 0;
      telefono->pedido= -1;
      printf("\tDueño llamando para cerrar local\n");
    }

    sem_post(telefono->semaforoLlamadas);
  }
  pthread_exit(NULL);
}

void TimeOut() {
  timeout = 0;
}

// Hilo Encargado
void atenderPedido(Encargado * encargado){
  int error = 0;
  // Verifica si hay alguna llamada entrante
  error = sem_trywait(encargado->telefono->semaforoLlamadas);
  if(!error){
    printf("\t\ttelefono atendido\n");
    usleep(rand()% 100001 + 250000);
    int codigoPedido = encargado->telefono->pedido;
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
    printf("\t\tPedido %d de la carta cargado\n", codigoPedido);

  error = GuardarDato(encargado->monitorComandas, codigoPedido);
  if(error)
    perror("GuardarDato()");
}

void cobrarPedido(Encargado * encargado){
  int cobrosPendientes = 0;

  // Se fija si hay algun delivery esperando para que le cobre
  sem_getvalue(encargado->memoria->semaforoPedidosPorCobrar, &cobrosPendientes);
  if(cobrosPendientes > 0){
    sem_post(encargado->memoria->semaforoDejarDinero);
    sem_wait(encargado->memoria->semaforoCobrarDinero);
    if( encargado->memoria->dato != -1) {
      printf("\t\t$%.0f guardados de pedido %d\n", precios[encargado->memoria->dato], encargado->memoria->dato);
    }
    else {
      printf("\t\tCerrando local\n");
      encargado->ultimoPedido = -1;
    }
    sem_trywait(encargado->memoria->semaforoPedidosPorCobrar);
  }
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
  Cocinero *cocinero = (Cocinero *) tmp;
  int * terminado = (int *)(calloc(1, sizeof(int)));
  while(*terminado != -1) {
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

  // Agregamos condicion para que no se mapee 2 veces
  if(delivery->memoria == NULL)
    delivery->memoria = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, delivery->ubiMemoria, 0);

  // El Delivery trabaja
  while(*terminado != -1)
    repartirPedido(delivery, terminado);

  // Desmapeo la memoria, si es el ultimo delivery
  if(delivery->cantDeliveries == 0){
      if (delivery->memoria != NULL) {
        int error = munmap((void*)(delivery->memoria), 2 * sizeof(Memoria));
        if (error)
          perror("delivery_munmap()");
      }
  }

  // Termino el hilo
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
    if(delivery->cantDeliveries == 0)
      avisarCobro(delivery, -1);
  }
}

void avisarCobro(Delivery * delivery, int pedidoCobrar){
  sem_post(delivery->memoria->semaforoPedidosPorCobrar);
  sem_wait(delivery->memoria->semaforoDejarDinero);
  if(pedidoCobrar != -1) {
    printf("\t\t\t\tdejando dinero de pedido %d\n", pedidoCobrar);
  }
  usleep(100000);
  delivery->memoria->dato = pedidoCobrar;
  sem_post(delivery->memoria->semaforoCobrarDinero);
}

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de Telefono
Telefono * crearTelefono() {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  telefono->semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(Telefono *telefono, struct Monitor_t * monitorComandas, int memoria){
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->telefono = telefono;
  encargado->monitorComandas = monitorComandas;
  encargado->ubiMemoria = memoria;
  encargado->memoria = NULL;
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
Delivery * crearDelivery(struct Monitor_t * monitorPedidos, int memoria) {
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->monitorPedidos = monitorPedidos;
  delivery->cantDeliveries = DELIVERIES;
  delivery->ubiMemoria = memoria;
  delivery->memoria = NULL;
  return delivery;
}

// Creacion de Memoria
int crearMemoria() {
  int error = 0;

  // Creo la memoria
  int ubiMemoria = shm_open("/memCompartida", O_CREAT | O_RDWR, 0660);
  if (ubiMemoria < 0) {
    perror("shm_open()");
    error = -1;
  }
  if (!error) {
    error = ftruncate(ubiMemoria, sizeof(Memoria));
    if (error)
      perror("ftruncate()");
  }

  // Lleno la memoria
  llenarMemoria(ubiMemoria);

  // Devuelvo la ubicacion de la memoria.
  return ubiMemoria;
}

void llenarMemoria(int ubicacion) {
  // Mapeo una referencia a la memoria e inicializo la estructura que lleva adentro.
  Memoria * temp = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, ubicacion, 0);

  temp->semaforoPedidosPorCobrar = (sem_t *)(calloc(1, sizeof(sem_t)));
  temp->semaforoPedidosPorCobrar = sem_open("/semPedidosPorCobrar", O_CREAT, O_RDWR, 0);

  temp->semaforoDejarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  temp->semaforoDejarDinero = sem_open("/semDejarDinero", O_CREAT, O_RDWR, 0);

  temp->semaforoCobrarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  temp->semaforoCobrarDinero = sem_open("/semCobrarDinero", O_CREAT, O_RDWR, 0);

  temp->dato = 0;

  // Desmapeo la referencia.
  if (temp != NULL) {
    int error = munmap((void*)(temp), 2 * sizeof(Memoria));
    if (error) {
      perror("creacion_memoria_munmap()");
    }
  }
}

/*-----------------FUNCIONES DE LIBERACION DE MEMORIA---------------------*/

// Borrado de semaforos y memoria
void borrarSemMem(Encargado * enc) {
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
  status = sem_close(enc->memoria->semaforoPedidosPorCobrar);
  if (!status) {
    status = sem_unlink("/semPedidosPorCobrar");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo DejarDinero
  status = sem_close(enc->memoria->semaforoDejarDinero);
  if (!status) {
    status = sem_unlink("/semDejarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo TomarDinero
  status = sem_close(enc->memoria->semaforoCobrarDinero);
  if (!status) {
    status = sem_unlink("/semCobrarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Desmapeo la memoria
  if (enc->memoria != NULL) {
    int error = munmap((void*)(enc->memoria), 2 * sizeof(Memoria));
    if (error) {
      perror("encargado_munmap()");
    }
  }

  // Memoria Compartida
  if (enc->ubiMemoria > 0) {
    status = shm_unlink("/memCompartida");
    if (status) {
      perror("unlink()");
    }
  }
}
