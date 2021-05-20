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
#include "conio.h"

// colores
#define RESET_COLOR  "\x1b[0m"
#define NEGRO_T      "\x1b[30m"
#define ROJO_F       "\x1b[41m"
#define VERDE_F      "\x1b[42m"
#define AMARILLO_F   "\x1b[43m"
#define MAGENTA_F    "\x1b[45m"
#define CYAN_T       "\x1b[36m"
#define CYAN_F       "\x1b[46m"
#define BLANCO_T     "\x1b[37m"
#define BLANCO_F     "\x1b[47m"

// Cantidad de actores
#define ENCARGADOS 1
#define COCINEROS 3
#define DELIVERIES 2
#define BUFFERCOMANDAS 4
#define BUFFERPEDIDOS 4

// Datos de entrada
#define CARTA 5
#define ALARMA 10

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
  int * puntuacion;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct{
  Telefono * telefono;
  struct Monitor_t *monitorComandas;
  // int ultimoPedido;
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
Telefono * crearTelefono(int *);
Encargado * crearEncargado(Telefono *, struct Monitor_t *, int);
Cocinero * crearCocinero(struct Monitor_t *, struct Monitor_t *);
Delivery * crearDelivery(struct Monitor_t *, int);

// Memoria
int crearMemoria();
void llenarMemoria(int);

/*-------------------------FUNCIONES DEL JUGADOR------------------------------*/
void mostrarMenu();
int comenzarJuego();
void jugar();
int  chequearPuntuacion(int); // Aca chequea si el score entra en el top 10
void guardarPuntuacion(int);  // Lo guarda
void verPuntuacion();
void salir();

/*---------------------FUNCIONES DE ACTORES DEL JUEGO-------------------------*/
//Funciones de telefono
void * gestionTelefono( void *);
void TimeOut();

//Funciones de encargado
void atenderPedido(Encargado *);
void cargandoPedido(Encargado *, int);
void cobrarPedido(Encargado *, int *);

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
void borrarSemMem(Encargado *, int);

/*-----------------------------------------------------------------------------*/
/*----------------------------------MAIN---------------------------------------*/

int main(){
  srand(time(NULL));

  // Interfaz de usuario
  int terminar = 1;
  char eleccion;
  do
  {
    mostrarMenu();

    do{
      eleccion = getch();
      if(eleccion != '1' && eleccion != '2' && eleccion != '3') {
        printf("\nOpcion invalida, por favor seleccion 1 2 o 3\nIngrese una opcion: ");
      }
    }while(eleccion != '1' && eleccion != '2' && eleccion != '3');

    switch (eleccion)
    {
    case '1':
        system("clear");
        // terminar = 0;
        int puntuacion = comenzarJuego();;
        if(chequearPuntuacion(puntuacion))
          guardarPuntuacion(puntuacion);
        break;
    case '2':
        system("clear");
        verPuntuacion();
        break;
    case '3':
        terminar = 0;
        salir();
        system("clear");
        break;
    case '4':
        system("clear");
        guardarPuntuacion(10);
        break;
    default:
      break;
    }
  } while (terminar);

  return 0;
}

void mostrarMenu() {
  system("clear");
  printf(NEGRO_T BLANCO_F"|------------------------------------|"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|-------------"BLANCO_T MAGENTA_F" PIZZERIA "NEGRO_T BLANCO_F"-------------|"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|                                    |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"1. Comenzar juego."NEGRO_T BLANCO_F"         |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"2. Ver puntuacion."NEGRO_T BLANCO_F"         |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"3. Salir."NEGRO_T BLANCO_F"                  |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|                                    |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|************************************|"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|             "BLANCO_T MAGENTA_F"Como jugar"NEGRO_T BLANCO_F"             |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|                                    |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"a: Atender telefono"NEGRO_T BLANCO_F"        |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"s: Cobrar pedido"NEGRO_T BLANCO_F"           |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|                                    |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|------------------------------------|"RESET_COLOR"\n");
  printf("Ingrese una opcion: ");
}

int comenzarJuego(){
  int puntuacion = 0;

  // Creamos los monitores
  struct Monitor_t * monitorComandas = CrearMonitor(BUFFERCOMANDAS);
  struct Monitor_t * monitorPedidos = CrearMonitor(BUFFERPEDIDOS);

  // Creamos la memoria y traemos su ubiacion
  int memoria =  crearMemoria();

  // Se crean los actores del juego
  Telefono * telefono = crearTelefono(&puntuacion);
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
  encargado->memoria = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, encargado->ubiMemoria, 0);

  // arranca el ciclo de juego del encargado, donde se detecta las teclas que presiona
  jugar(encargado);

  //Desmapeo la memoria del encargado
  if (encargado->memoria != NULL) {
    int error = munmap((void*)(encargado->memoria), 2 * sizeof(Memoria));
    if (error) {
      perror("encargado_munmap()");
    }
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
  borrarSemMem(encargado, memoria);

  // Borramos los monitores
  BorrarMonitor(monitorComandas);
  BorrarMonitor(monitorPedidos);

  // Se libera la memoria de los objetos creados
  free(telefono);
  free(encargado);
  free(cocinero);
  free(delivery);

  return puntuacion;
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
  /* if( codigoPedido != -1)
    printf("\t\tPedido %d de la carta cargado\n", codigoPedido); */

  error = GuardarDato(encargado->monitorComandas, codigoPedido);
  if(error)
    perror("GuardarDato()");
}

void cobrarPedido(Encargado * encargado, int * terminado){
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
      * terminado = -1;
    }
    sem_trywait(encargado->memoria->semaforoPedidosPorCobrar);
  }
}

// Hilo telefono
void * gestionTelefono(void * tmp){
  timeout = 1;
  Telefono *telefono = (Telefono *) tmp;

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);

  while(timeout) {
    sem_wait(telefono->semaforoTelefono);
    usleep(rand()% 750001 + 250000);

    telefono->pedido = rand() % CARTA;
    printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");

    sem_post(telefono->semaforoLlamadas);
    sleep(1);
    int telefonoSonando = 0;
    sem_getvalue(telefono->semaforoLlamadas, &telefonoSonando);
    if(telefonoSonando == 1){
      int error = 0;
      error = sem_trywait(telefono->semaforoLlamadas);
      if(!error){
        printf("\tSe perdio la llamada\n");
        sem_post(telefono->semaforoTelefono);
      }
    }else{
      *telefono->puntuacion = *telefono->puntuacion+1;
    }
  }

  // Envia el último pedido
  sem_wait(telefono->semaforoTelefono);
  telefono->pedido= -1;
  printf(BLANCO_T ROJO_F"\tDueño llamando para cerrar local"RESET_COLOR"\n");
  sem_post(telefono->semaforoLlamadas);

  // printf("Telefono terminado\n");

  // Termina el hilo
  pthread_exit(NULL);
}

void TimeOut() {
  timeout = 0;
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {
  Cocinero *cocinero = (Cocinero *) tmp;
  int * terminado = (int *)(calloc(1, sizeof(int)));
  while(*terminado != -1) {
    cocinarPedido(cocinero, terminado);
  }
  free(terminado);
  // printf("Cocinero terminado\n");
  // Termino el hilo
  pthread_exit(NULL);
}

void cocinarPedido(Cocinero * cocinero, int * terminado) {
  int error = 0;
  int pedidoActual = 0;
  error = LeerDato(cocinero->monitorComandas, &pedidoActual);
  if(error)
    perror("LeerDato()");
  else {
    // Si el pedido actual es -1, entonces empieza a cerrar la cocina
    if( pedidoActual != -1) {
      //printf("\t\t\tcocinando pedido %d\n", pedidoActual);
      usleep(rand()% 500001 + 1000000);
      //printf("\t\t\tpedido %d cocinado\n", pedidoActual);
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
  // if( pedidoListo != -1)
    //printf("\t\t\tPedido %d listo para ser repartido\n", pedidoListo);
    
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
  while(*terminado != -1){
    repartirPedido(delivery, terminado);
  }

  // Desmapeo la memoria, si es el ultimo delivery
  if(delivery->cantDeliveries == 0){
      if (delivery->memoria != NULL) {
        int error = munmap((void*)(delivery->memoria), 2 * sizeof(Memoria));
        if (error)
          perror("delivery_munmap()");
      }
  }

  free(terminado);
  // printf("Delivery terminado\n");
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
      //printf("\t\t\t\trepartiendo pedido %d\n", pedidoRepartir);
      usleep(rand()% 250001 + 350000);
      // printf("\t\t\t\tpedido %d entregado\n", pedidoRepartir);
      usleep(rand()% 250001 + 350000);
      avisarCobro(delivery, pedidoRepartir);
    }
    else {
      // printf("\t\t\t\tDelivery recibe -1\n");
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
  if(pedidoCobrar != -1) {
    printf(NEGRO_T AMARILLO_F"\t\t\t\tdejando dinero de pedido %d"RESET_COLOR"\n", pedidoCobrar);
  }else{
    sleep(1);
    printf(BLANCO_T VERDE_F"\t\t\t\tpresione s para cerrar el local"RESET_COLOR"\n");
  }
  sem_wait(delivery->memoria->semaforoDejarDinero);
  usleep(100000);
  delivery->memoria->dato = pedidoCobrar;
  sem_post(delivery->memoria->semaforoCobrarDinero);
}

/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de Telefono
Telefono * crearTelefono(int * puntuacion) {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  telefono->semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  telefono->puntuacion = puntuacion;
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
void borrarSemMem(Encargado * enc, int ubiMemoria) {
  Memoria * temp = mmap(NULL, sizeof(Memoria), PROT_READ | PROT_WRITE, MAP_SHARED, ubiMemoria, 0);
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
  status = sem_close(temp->semaforoPedidosPorCobrar);
  if (!status) {
    status = sem_unlink("/semPedidosPorCobrar");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo DejarDinero
  status = sem_close(temp->semaforoDejarDinero);
  if (!status) {
    status = sem_unlink("/semDejarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo CobrarDinero
  status = sem_close(temp->semaforoCobrarDinero);
  if (!status) {
    status = sem_unlink("/semCobrarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Desmapeo la memoria
  if (temp != NULL) {
    int error = munmap((void*)(temp), 2 * sizeof(Memoria));
    if (error) {
      perror("temp_munmap()");
    }
  }

  // Memoria Compartida
  if (ubiMemoria > 0) {
    status = shm_unlink("/memCompartida");
    if (status) {
      perror("unlink()");
    }
  }
}

/*-------------------FUNCIONES DE LA PUNTUACION---------------------------*/

int  chequearPuntuacion(int score) {
  return 1;
}

void guardarPuntuacion(int score) {
  FILE * archivoPuntuacion;
  archivoPuntuacion = fopen("./puntuacion.txt", "a");
  if(archivoPuntuacion == NULL)
    perror("fopen()");
  
  printf("Escriba su nombre: ");
  char * nombre = (char*)(calloc(20,sizeof(char)));
  scanf("%s", nombre);

  fprintf(archivoPuntuacion,"%s: %d\n", nombre, score);
  fflush(archivoPuntuacion);

  free(nombre);
  int error = fclose(archivoPuntuacion);
  if(error)
    perror("fclose()");
}

void verPuntuacion(){
  FILE * archivoPuntuacion;
  archivoPuntuacion = fopen("./puntuacion.txt", "r");
  if(archivoPuntuacion == NULL)
    perror("fopen()");
  
  char * nombre = (char*)(calloc(20,sizeof(char)));
  int  * score  = (int *)(calloc(1,sizeof(int)));;

  int leido = fscanf(archivoPuntuacion, "%s %d", nombre, score);
  while(leido != EOF) {
    leido = fscanf(archivoPuntuacion, "%s %d", nombre, score);
    printf("%s %d\n", nombre, *score);
  }

  int temp = getchar();

  free(nombre);
  free(score);

  int error = fclose(archivoPuntuacion);
  if(error)
    perror("fclose()");
}

void jugar(Encargado * encargado){
  int * terminado = (int *)(calloc(1, sizeof(int)));
  char eleccion;
  while(* terminado != -1){
    do
    {
      eleccion = getchar();
    } while (eleccion != 'a' && eleccion != 's' && eleccion != 'd');
    switch (eleccion){
    case 'a':
      atenderPedido(encargado);
      break;
    case 's':
      cobrarPedido(encargado, terminado);
      break;
    default:
      break;
    }
  }
  free(terminado);
}

void salir(){

}
