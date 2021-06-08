#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

// COLORES
#define RESET_COLOR  "\x1b[0m"
#define NEGRO_T      "\x1b[30m"
#define ROJO_F       "\x1b[41m"
#define VERDE_F      "\x1b[42m"
#define AMARILLO_F   "\x1b[43m"
#define MAGENTA_F    "\x1b[45m"
#define BLANCO_T     "\x1b[37m"
#define BLANCO_F     "\x1b[47m"

// DATOS DEL JUEGO
#define ENCARGADOS 1
#define COCINEROS 3
#define DELIVERIES 2
#define CARTA 5
#define ALARMA 10
#define ULTIMOPEDIDO -1

int timeout = 1; // INDICA CUANDO EL JUEGO VA A TERMINAR

// ESTRUCTURA DEL TELEFONO
typedef struct {
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  int * tubo;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  Telefono * comTel;
  char pedidoActual [3];
  int puntuacion;
  int comandaEnMano;
  mqd_t enviar;
  sem_t * semaforoPedidosPorCobrar;
  sem_t * semaforoDejarDinero;
  sem_t * semaforoCobrarDinero;
  int fifoOut;
  float precios[CARTA];
}Encargado;

// ESTRUCTURA DEL COCINERO
typedef struct {
  mqd_t recibir;
  mqd_t enviar;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL DELIVERY
typedef struct {
  mqd_t recibir;
  sem_t * semaforoPedidosPorCobrar;
  sem_t * semaforoDejarDinero;
  sem_t * semaforoCobrarDinero;
  int fifoIn;
  int cantDelivery;
}Delivery;

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// ACTORES
Telefono  * crearTelefono();
Encargado * crearEncargado(Telefono *, mqd_t, sem_t *, sem_t *, sem_t *);
Cocinero  * crearCocinero(mqd_t, mqd_t);
Delivery  * crearDelivery(mqd_t, sem_t *, sem_t *, sem_t *);

/*----------------------------------------------------------------------------*/
/*-------------------------FUNCIONES DEL JUGADOR------------------------------*/
void mostrarMenu();
int comenzarJuego();
void jugar();
void guardarPuntuacion(int);
void verPuntuacion();
void salir(int *);

/*----------------------------------------------------------------------------*/
/*---------------------FUNCIONES DE ACTORES DEL JUEGO-------------------------*/
// FUNCIONES DEL TELEFONO
void gestionTelefono( void *);
void recibirLlamada(Telefono *);
void hacerPedido(Telefono *);
void hacerUltimoPedido(Telefono *);
void TimeOut();

// FUNCIONES DEL ENCARGADO
void atenderPedido(Encargado *);
void cargarPedido(Encargado *);
void cobrarPedido(Encargado *, int *);

// FUNCIONES DEL COCINERO
void hilosCocineros(Cocinero *);
void * gestionCocinero(void *);
void cocinarPedido(Cocinero *, int *);
void pedidoListo(Cocinero *, char *);

// FUNCIONES DEL DELIVERY
void hilosDelivery(Delivery *);
void * gestionDelivery(void *);
void repartirPedido(Delivery *, int *);
void avisarCobro(Delivery *, char *);

/*-----------------------------------------------------------------------------*/
/*----------------------------------MAIN---------------------------------------*/
int main(){

  int terminar = 1;
  char eleccion;
  do
  {
    mostrarMenu(); // INTERFAZ DE USUARIO

    do{
      eleccion = getchar();
      __fpurge(stdin);
      if(eleccion != '1' && eleccion != '2' && eleccion != '3') {
        printf("\nOpcion invalida, por favor seleccion 1 2 o 3\nIngrese una opcion: ");
      }
    }while(eleccion != '1' && eleccion != '2' && eleccion != '3');


    switch (eleccion)
    {
    case '1':
        system("clear");
        int puntuacion = comenzarJuego();
        guardarPuntuacion(puntuacion);
        break;
    case '2':
        system("clear");
        verPuntuacion();
        break;
    case '3':
        system("clear");
        salir(&terminar);
        break;
    default:
      break;
    }
  } while (terminar);

  return 0;
}

void jugar(Encargado * encargado){
  int * terminado = (int *)(calloc(1, sizeof(int)));
  char eleccion;
  while(* terminado != -1){
    do {
      eleccion = getchar();
    } while (eleccion != 'a' && eleccion != 's' && eleccion != 'd');

    switch (eleccion){
    case 'a':
      atenderPedido(encargado);
      break;
    case 's':
      cargarPedido(encargado);
      break;
    case 'd':
      cobrarPedido(encargado, terminado);
      break;
    default:
      break;
    }
  }
  free(terminado);
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
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"s: Cargar pedido"NEGRO_T BLANCO_F"           |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|         "NEGRO_T BLANCO_F"d: Cobrar pedido"NEGRO_T BLANCO_F"           |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|                                    |"RESET_COLOR"\n");
  printf(NEGRO_T BLANCO_F"|------------------------------------|"RESET_COLOR"\n");
  printf("Ingrese una opcion: ");
}

int comenzarJuego(){
  // Creamos las colas de mensajes para enc-coc y coc-del
  struct mq_attr attr;  
  attr.mq_flags = 0;  
  attr.mq_maxmsg = 10;  
  attr.mq_msgsize = 4;  
  attr.mq_curmsgs = 0;
  mqd_t mqdComandasEnc, mqdComandasCoc, mqdPedidosCoc, mqdPedidosDel;
  mqdComandasEnc = mq_open("/encargadoCocineros",O_WRONLY | O_CREAT, 0777, &attr);
  mqdComandasCoc = mq_open("/encargadoCocineros",O_RDONLY, 0777, &attr);
  mqdPedidosCoc  = mq_open("/cocinerosDelivery",O_WRONLY | O_CREAT, 0777, &attr);
  mqdPedidosDel  = mq_open("/cocinerosDelivery",O_RDONLY, 0777, &attr);

  // Creamos una fifo para del-enc. Tmb los semaforos
  int error = 0;
  error = mkfifo("/tmp/deliveryEncargado", 0777);
  if ((error) && (errno!=EEXIST)) {
    perror("mkfifo");
  }

  sem_t * semaforoPedidosPorCobrar = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoPedidosPorCobrar = sem_open("/semPedidosPorCobrar", O_CREAT, O_RDWR, 0);

  sem_t * semaforoDejarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoDejarDinero = sem_open("/semDejarDinero", O_CREAT, O_RDWR, 0);

  sem_t * semaforoCobrarDinero = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoCobrarDinero = sem_open("/semCobrarDinero", O_CREAT, O_RDWR, 0);

  // Se crean los actores del juego donde pasamos lo que creamos recien
  Telefono  * telefono  = crearTelefono();
  Encargado * encargado = crearEncargado(telefono, mqdComandasEnc, semaforoPedidosPorCobrar, semaforoDejarDinero, semaforoCobrarDinero);
  Cocinero  * cocinero  = crearCocinero(mqdComandasCoc, mqdPedidosCoc);
  Delivery  * delivery  = crearDelivery(mqdPedidosDel, semaforoPedidosPorCobrar, semaforoDejarDinero, semaforoCobrarDinero);

  pid_t pid;

  pid = fork();
  if(pid == 0) {
    gestionTelefono(telefono);
    exit(0);
  }

  pid = fork();
  if(pid == 0){
    hilosCocineros(cocinero);
    exit(0);
  }

  pid = fork();
  if(pid == 0){
    hilosDelivery(delivery);
    exit(0);
  }

  // Arranca el ciclo de juego del encargado, donde se detecta las teclas que presiona
  if(pid > 0){
      close(encargado->comTel->tubo[1]);
      encargado->fifoOut=open("/tmp/deliveryEncargado",O_RDONLY);
      if (encargado->fifoOut<0) {
        error=encargado->fifoOut;
        perror("ENCARGADO fifo open");
      }
      jugar(encargado);
      close(encargado->comTel->tubo[0]);
  }
  
  // Se liberan los semaforos
  // borrarSemMem(encargado, memoria);

  int status=0;
  // Semaforo Telefono
  status = sem_close(encargado->comTel->semaforoTelefono);
  if (!status) {
    status = sem_unlink("/semTelefono");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  status = sem_close(encargado->comTel->semaforoLlamadas);
  if (!status) {
    status = sem_unlink("/semLlamadas");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  status = mq_close(cocinero->recibir);
  status = mq_close(encargado->enviar);
  if (!status) {
    status = mq_unlink("/encargadoCocineros");
    if (status)
      perror("mq_close()");
  }

  status = mq_close(cocinero->enviar);
  status = mq_close(delivery->recibir);
  if (!status) {
    status = mq_unlink("/cocinerosDelivery");
    if (status)
      perror("mq_close()");
  }

  status = unlink("/tmp/deliveryEncargado");
  if(status) {
    perror("unlink");
  }

  status = sem_close(semaforoPedidosPorCobrar);
  if (!status) {
    status = sem_unlink("/semPedidosPorCobrar");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo DejarDinero
  status = sem_close(semaforoDejarDinero);
  if (!status) {
    status = sem_unlink("/semDejarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Semaforo CobrarDinero
  status = sem_close(semaforoCobrarDinero);
  if (!status) {
    status = sem_unlink("/semCobrarDinero");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  // Se libera la memoria de los objetos creados
  // free(telefono);
  // free(encargado);
  // free(cocinero);
  // free(delivery);

  return encargado->puntuacion;
}

// Hilo Encargado
void atenderPedido(Encargado * encargado) {
  // Si tiene una comanda en la mano, no puede atender el telefono
  if(encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tdejar comanda antes de atender otro pedido"RESET_COLOR"\n");
    return;
  }

  // Verifica si hay alguna llamada entrante
  int error = sem_trywait(encargado->comTel->semaforoLlamadas);
  if(!error) {
    sem_post(encargado->comTel->semaforoTelefono);
    read(encargado->comTel->tubo[0], encargado->pedidoActual, 3);
    encargado->comandaEnMano = 1; // Flag, encargado debe cargar el pedido antes de atender otro
    usleep(rand()% 50001 + 50000); // Tiempo que tarda en tomar el pedido

    if(strcmp(encargado->pedidoActual, "-1")){
        printf(BLANCO_T MAGENTA_F"\t\tcomanda de pedido %s lista para cargar"RESET_COLOR"\n", 
            encargado->pedidoActual);
      encargado->puntuacion++;
      sem_post(encargado->comTel->semaforoTelefono); // Cuelga el telefono
    }
    else {
      printf(BLANCO_T MAGENTA_F"\t\tavisar a los cocineros que cierren la cocina"RESET_COLOR"\n");
    }
  }
}

void cargarPedido (Encargado * encargado) {
  // Si no tiene una comanda en la mano, no puede cargar ningun pedido
  if(!encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tatender el telefono antes de cargar un pedido"RESET_COLOR"\n");
    return;
  }

  // Deja comanda a los cocineros
  if(strcmp(encargado->pedidoActual, "-1")) {
    int enviado=mq_send(encargado->enviar,encargado->pedidoActual,strlen(encargado->pedidoActual)+1,0);
    if (enviado == -1)
      perror("ENCARGADO mq_send");
  }
  else {
    for (int i = 0; i < COCINEROS; i++){
      // Si el pedido actual es el ultimo, avisa a cada cocinero que cierren la cocina
      int enviado=mq_send(encargado->enviar,encargado->pedidoActual,strlen(encargado->pedidoActual)+1,0);
      if (enviado==-1)
        perror("ENCARGADO mq_send");
    }
  }
  encargado->comandaEnMano = 0;
}

// Hilo Telefono
void gestionTelefono(void * tmp){
  Telefono * telefono = (Telefono *) tmp;
  
  // Cerramos el canal de lectura
  close(telefono->tubo[0]); 

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);

  // Ciclo de recibir llamadas, finaliza cuando suena la alarma
  while (timeout){
    // El telefono comienza a recibir llamadas mientras esta colgado
    sem_wait(telefono->semaforoTelefono);
    usleep(rand() % 750001 + 250000);
    recibirLlamada(telefono);
  }

  // Envia el último pedido
  hacerUltimoPedido(telefono);

  // Cerramos el pipe
  close(telefono->tubo[1]);
}

void recibirLlamada(Telefono * telefono) {
  int error = 0;

  // El telefono comienza a sonar
  printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");
  sem_post(telefono->semaforoLlamadas);
  
  // El cliente esta 1 segundo esperando ser atendido
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 1;
  error = sem_timedwait(telefono->semaforoTelefono, &ts);

  // Si no se atiende a tiempo
  if(error) {
    error = sem_trywait(telefono->semaforoLlamadas);
    // En caso de tener la mala suerte que el encargado logra tomar
    // el semaforoLlamadas luego del timedwait y antes del trywait de arriba
    // hay que hacer de cuenta que si atendio el telefono y seguir el curso
    // normal. El encargado hizo post sobre semaforoTelefono.
    if(error) {
      sem_trywait(telefono->semaforoTelefono);
      hacerPedido(telefono);
    }
    else {
      printf("\tSe perdio la llamada\n");
      sem_post(telefono->semaforoTelefono);
    }
  }
  else
    hacerPedido(telefono);
}

void hacerPedido(Telefono * telefono) {
  char numeroPedido [2];

  printf("\tLlamada atendida\n");
  snprintf(numeroPedido, 2, "%d", rand() % CARTA);
  usleep(rand()% 50001 + 50000); // Tiempo que tarda en pedir
  write(telefono->tubo[1], numeroPedido, 2);
}

void hacerUltimoPedido(Telefono * telefono) {
  char cadena[3];

  // El telefono comienza a sonar
  printf(BLANCO_T ROJO_F"\tDueño llamando para cerrar local"RESET_COLOR"\n");
  sem_post(telefono->semaforoLlamadas);
  sem_wait(telefono->semaforoTelefono);

  // Envia el ultimo pedido
  snprintf(cadena, 3, "%d", ULTIMOPEDIDO);
  write(telefono->tubo[1], cadena, 3);
}

void TimeOut() {
  timeout = 0;
}

// Hilo Cocinero
void hilosCocineros(Cocinero * cocinero) {
  pthread_t hilosCocineros[COCINEROS];
  for(int i = 0; i < COCINEROS; i++) 
    pthread_create(&hilosCocineros[i], NULL, gestionCocinero, (void *)(cocinero));
  for(int i = 0; i < COCINEROS; i++)
    pthread_join(hilosCocineros[i], NULL);
}

void * gestionCocinero(void * tmp) {
  Cocinero *cocinero = (Cocinero *) (tmp);
  int * terminado = (int *)(calloc(1, sizeof(int)));

  // Ciclo de cocinar pedidos, finaliza cuando el encargado avisa
  while(*terminado != -1)
    cocinarPedido(cocinero, terminado);
    
  free(terminado);

  pthread_exit(NULL);
}

void cocinarPedido(Cocinero * cocinero, int * terminado) {
  // Toma una comanda para empezar a cocinar
  char pedido[5];
  int recibido = mq_receive(cocinero->recibir, pedido, 5, NULL);
  // usleep(rand() % 1000001 + 500000);
  if (recibido == -1)
    perror("COCINERO mq_receive");
  else{
    if(strcmp(pedido, "-1")) {
      usleep(rand()% 500001 + 1000000); // Tiempo que se demora en cocinar
      pedidoListo(cocinero, pedido);
    }
    else{
      cocinero->cantCocineros--;
      * terminado = -1;
      // El ultimo cocinero es el encargado de avisar a los deliveries que ya cerró la cocina
      if(cocinero->cantCocineros == 0) {
        for(int i = 0; i < DELIVERIES; i++) {
          pedidoListo(cocinero, "-1");
          printf("\t\t\tSe va el cocinero\n");
        }
      }
    }
  }
}

void pedidoListo(Cocinero * cocinero, char * pedido) {
  // Deja el pedido en el mostrador
  if(strcmp(pedido, "-1")) {
    int enviado=mq_send(cocinero->enviar,pedido,strlen(pedido)+1,0);
    printf("\t\t\tPedido %s cargado\n", pedido);
    if (enviado == -1)
      perror("COCINERO mq_send");
  }
  else {
    for (int i = 0; i < DELIVERIES; i++){
      // Si el pedido actual es el ultimo, avisa a cada delivery que se vayan
      int enviado=mq_send(cocinero->enviar,pedido,strlen(pedido)+1,0);
      if (enviado==-1)
        perror("COCINERO mq_send");
    }
  }
}


// Hilo Delivery
void hilosDelivery(Delivery * delivery) {
  pthread_t hilosDelivery[DELIVERIES];
  for(int i = 0; i < DELIVERIES; i++) 
    pthread_create(&hilosDelivery[i], NULL, gestionDelivery, (void *)(delivery));
  for(int i = 0; i < DELIVERIES; i++)
    pthread_join(hilosDelivery[i], NULL);
}

void * gestionDelivery(void * tmp) {
  Delivery * delivery = (Delivery *)(tmp);
  int * terminado = (int *)(calloc(1, sizeof(int)));

  // Abrimos la fifo como escritura una vez por cada hilo delivery
  delivery->fifoIn = open("/tmp/deliveryEncargado",O_WRONLY);
  if (delivery->fifoIn < 0) {
    perror("DELIVERY fifo open");
  }

  // Ciclo de repartir pedidos
  while(*terminado != -1)
    repartirPedido(delivery, terminado);

  free(terminado);

  // Termino el hilo
  pthread_exit(NULL);
}

void repartirPedido(Delivery * delivery, int * terminado) {
  // Toma una pedido listo para repartir al cliente
  char pedido[5];
  int recibido = mq_receive(delivery->recibir, pedido, 5, NULL);
  if (recibido == -1)
    perror("DELIVERY mq_receive");
  else{
    if(strcmp(pedido, "-1")) {
      usleep(rand()% 500001 + 500000);  // Tiempo que se demora en repartir pedido
      printf("\t\t\t\tPedido %s repartido\n", pedido);
      avisarCobro(delivery, pedido);
    }
     else {
      delivery->cantDelivery--;
      * terminado = -1;
      // Si es el ultimo delivery que termina, le avisa al encargado
      if(delivery->cantDelivery == 0)
      avisarCobro(delivery, "-1");
    }
  }
}

void avisarCobro(Delivery * delivery, char * pedidoCobrar){

  // Avisa al encargado que esta listo para dejar dinero
  sem_post(delivery->semaforoPedidosPorCobrar);

  // Deja el dinero
  if(strcmp(pedidoCobrar, "-1")) 
    printf(NEGRO_T AMARILLO_F"\t\t\t\tPedido %s listo para cobrar"RESET_COLOR"\n", pedidoCobrar);  
  // Avisa que se va
  else 
    printf(BLANCO_T VERDE_F"\t\t\t\tPresione d para cerrar el local"RESET_COLOR"\n");
  
  sem_wait(delivery->semaforoDejarDinero);
  write(delivery->fifoIn, pedidoCobrar, 3); // escribimos en la filo el pedido por cobrar
  sem_post(delivery->semaforoCobrarDinero);
}

void cobrarPedido(Encargado * encargado, int * terminado) {
  // Se fija si hay algun delivery esperando para que le cobre
  int cobrosPendientes = 0;
  int error = sem_getvalue(encargado->semaforoPedidosPorCobrar, &cobrosPendientes);
  if(!error) {
    if(cobrosPendientes > 0){
      sem_post(encargado->semaforoDejarDinero);
      sem_wait(encargado->semaforoCobrarDinero);
      char pedido[3];
      read(encargado->fifoOut,pedido,3);

      // Si el delivery le avisa que ya termino, cierra el local
      if(strcmp(pedido, "-1"))
        printf("\t\t$%.0f guardados de pedido %s\n", encargado->precios[atoi(pedido)], pedido);
      else {
        printf("\t\tCerrando local\n");
        * terminado = -1;
      }
      sem_trywait(encargado->semaforoPedidosPorCobrar);
    }
  }
  else{
    perror("encargado_sem_getvalue()");
  }
}

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de Telefono
Telefono * crearTelefono() {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  telefono->semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  telefono->tubo = (int *)(calloc(2, sizeof(int)));
  pipe(telefono->tubo);
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(Telefono * tel, mqd_t cola, sem_t * semaforoPedidosPorCobrar, sem_t * semaforoDejarDinero, sem_t * semaforoCobrarDinero) {
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->comTel = tel;
  encargado->enviar = cola;
  encargado->semaforoPedidosPorCobrar = semaforoPedidosPorCobrar;
  encargado->semaforoDejarDinero = semaforoDejarDinero;
  encargado->semaforoCobrarDinero = semaforoCobrarDinero;
  for(int i = 0; i < CARTA; i++)
    encargado->precios[i] = 100 * (i+1);
  return encargado;
}

// Creacion de Cocinero
Cocinero * crearCocinero(mqd_t recibir, mqd_t enviar) {
  Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
  cocinero->recibir = recibir;
  cocinero->enviar  = enviar;
  cocinero->cantCocineros = COCINEROS;
  return cocinero;
}

// Creacion de Delivery
Delivery  * crearDelivery(mqd_t recibir, sem_t * semaforoPedidosPorCobrar, sem_t * semaforoDejarDinero, sem_t * semaforoCobrarDinero) {
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->recibir = recibir;
  delivery->semaforoPedidosPorCobrar = semaforoPedidosPorCobrar;
  delivery->semaforoDejarDinero = semaforoDejarDinero;
  delivery->semaforoCobrarDinero = semaforoCobrarDinero;
  delivery->cantDelivery = DELIVERIES;
  return delivery;
}

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE LA PUNTUACION---------------------------*/
// Guarda la puntuacion
void guardarPuntuacion(int score) {
  FILE * archivoPuntuacion;
  archivoPuntuacion = fopen("./puntuacion.txt", "a");
  if(archivoPuntuacion == NULL)
    perror("fopen()");
  
  printf("Escriba su nombre: ");
  char * nombre = (char*)(calloc(20,sizeof(char)));
  scanf("%s", nombre);
  __fpurge(stdin);

  fprintf(archivoPuntuacion,"%s: %d\n", nombre, score);
  fflush(archivoPuntuacion);

  free(nombre);
  int error = fclose(archivoPuntuacion);
  if(error)
    perror("fclose()");
}

// Muestra la puntuacion historica
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
  __fpurge(stdin);
  if(temp){}

  free(nombre);
  free(score);

  int error = fclose(archivoPuntuacion);
  if(error)
    perror("fclose()");
}

// Cierra el juego
void salir(int * terminar){
  printf(MAGENTA_F BLANCO_T"\t\t\tGracias por jugar!!!"RESET_COLOR"\n");
  sleep(1);
  *terminar = 0;
  system("clear");
}