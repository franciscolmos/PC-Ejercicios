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

// LIBRERIAS PROPIAS
#include "DeleteCreate.h"

// COLORES
#define RESET_COLOR  "\x1b[0m"
#define NEGRO_T      "\x1b[30m"
#define ROJO_F       "\x1b[41m"
#define VERDE_F      "\x1b[42m"
#define AMARILLO_F   "\x1b[43m"
#define MAGENTA_F    "\x1b[45m"
#define BLANCO_T     "\x1b[37m"
#define BLANCO_F     "\x1b[47m"
#define CYAN_T       "\x1b[36m"
#define CYAN_F       "\x1b[46m"

// DATOS DEL JUEGO
#define ENCARGADOS 1
#define COCINEROS 3
#define DELIVERIES 2
#define CARTA 5
#define ALARMA 7
#define ULTIMOPEDIDO "-1"

int timeout = 1; // INDICA CUANDO EL JUEGO VA A TERMINAR

// ESTRUCTURA LA COMUNICACION ENCARGADO - DELIVERY
typedef struct {
  sem_t * semCobrarPedidos;
  int fifo;
}comDel;

// ESTRUCTURA DEL TELEFONO
typedef struct {
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  int * tubo;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  Telefono * comTel;
  mqd_t enviar;
  comDel * comDel;
  char pedidoActual [3];
  int comandaEnMano;
  float precios[CARTA];
  int puntuacion;
}Encargado;

// ESTRUCTURA DEL COCINERO
typedef struct {
  mqd_t recibir;
  mqd_t enviar;
  int cantCocineros;
}Cocinero;

// ESTRUCTURA DEL DELIVERY
typedef struct {
  comDel * comDel;
  mqd_t recibir;
  int cantDelivery;
}Delivery;

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// ACTORES
comDel    * crearComDel();
Telefono  * crearTelefono();
Encargado * crearEncargado(Telefono *, mqd_t, comDel *);
Cocinero  * crearCocinero(mqd_t, mqd_t);
Delivery  * crearDelivery(mqd_t, comDel *);

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
  mqd_t mqdComandasEnc = crearColaMensaje("encargadoCocineros", "enc", O_WRONLY | O_CREAT);
  mqd_t mqdComandasCoc = crearColaMensaje("encargadoCocineros", "coc", O_RDONLY);
  mqd_t mqdPedidosCoc  = crearColaMensaje("cocinerosDelivery" , "coc", O_WRONLY | O_CREAT);
  mqd_t mqdPedidosDel  = crearColaMensaje("cocinerosDelivery" , "del", O_RDONLY);

  // Se crean los actores del juego donde pasamos lo que creamos recien
  comDel    * comDel    = crearComDel();
  printf("Creado comDel\n");
  Telefono  * telefono  = crearTelefono();
  printf("Creado telefono\n");
  Encargado * encargado = crearEncargado(telefono, mqdComandasEnc, comDel);
  printf("Creado encargado\n");
  Cocinero  * cocinero  = crearCocinero(mqdComandasCoc, mqdPedidosCoc);
  printf("Creado cocinero\n");
  Delivery  * delivery  = crearDelivery(mqdPedidosDel, comDel);
  printf("Creado delivery\n");

  // CREAMOS LOS PROCESOS
  pid_t pid;

  pid = fork();
  if(pid == 0) {
    printf("Inicia gestion telefono\n");
    gestionTelefono(telefono); // TELEFONO
    exit(0);
  }

  pid = fork();
  if(pid == 0){
    printf("Inicia gestion cocinero\n");
    hilosCocineros(cocinero); // COCINEROS
    exit(0);
  }

  pid = fork();
  if(pid == 0){
    printf("Inicia gestion delivery\n");
    hilosDelivery(delivery); // DELIVERY
    exit(0);
  }

  // Arranca el ciclo de juego del encargado, donde se detecta las teclas que presiona
  if(pid > 0){
    close(encargado->comTel->tubo[1]);
    encargado->comDel->fifo = open("/tmp/deliveryEncargado",O_RDONLY);
    if (encargado->comDel->fifo<0)
      perror("fifo_open_enc()");
    printf("Inicia gestion encargado\n");
    jugar(encargado);
    close(encargado->comTel->tubo[0]);
  }

  // Borramos semaforos de telefono y de la comunicacion entre Encargado y Delivery
  printf("\n\n-----------------------------------------------------\n");
  borrarSemaforo(encargado->comTel->semaforoTelefono, "semTelefono",  "enc");
  borrarSemaforo(encargado->comDel->semCobrarPedidos, "semCobrarPedidos",  "enc");

  // Cerramos las colas de mensajes
  cerrarColaMensaje(cocinero->recibir, "encargadoCocineros", "coc");
  borrarColaMensaje(encargado->enviar, "encargadoCocineros", "enc");
  cerrarColaMensaje(delivery->recibir, "cocinerosDelivery",  "del");
  borrarColaMensaje(cocinero->enviar , "cocinerosDelivery",  "coc");

  // Cerramos FIFO
  borrarFIFO("deliveryEncargado");
  printf("-----------------------------------------------------\n\n");

  // Se libera la memoria de los objetos creados
  free(telefono);
  free(encargado);
  free(cocinero);
  free(delivery);

  return encargado->puntuacion;
}

// Hilo Encargado
void atenderPedido(Encargado * encargado) {
  if(encargado->comandaEnMano || strcmp(encargado->pedidoActual, ULTIMOPEDIDO) == 0) {
    printf(NEGRO_T BLANCO_F"\t\tdejar comanda antes de atender otro pedido"RESET_COLOR"\n");
    return;
  }

  // Verifica si hay alguna llamada entrante
  sem_post(encargado->comTel->semaforoTelefono);
  encargado->comandaEnMano++;
  read(encargado->comTel->tubo[0], encargado->pedidoActual, 3);

  if(strcmp(encargado->pedidoActual, ULTIMOPEDIDO)){
    printf(BLANCO_T MAGENTA_F"\t\tcomanda de pedido %s lista para cargar"RESET_COLOR"\n", 
            encargado->pedidoActual);
    encargado->puntuacion++;
    // sem_post(encargado->comTel->semaforoTelefono); // Cuelga el telefono
  }
  else
    printf(BLANCO_T MAGENTA_F"\t\tavisar a los cocineros que cierren la cocina"RESET_COLOR"\n");
}

void cargarPedido (Encargado * encargado) {
  // Si no tiene una comanda en la mano, no puede cargar ningun pedido
  if(!encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tatender el telefono antes de cargar un pedido"RESET_COLOR"\n");
    return;
  }

  // Deja comanda a los cocineros
  if(strcmp(encargado->pedidoActual, ULTIMOPEDIDO)) {
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
      else
        printf("Encargado_ultimoPedido_entregado()\n");
    }
  }
  encargado->comandaEnMano = 0;
}

void cobrarPedido(Encargado * encargado, int * terminado) {
  int cobrar = 0;
  sem_getvalue(encargado->comDel->semCobrarPedidos, &cobrar);
  if(cobrar){
    char pedido[3];
    sem_trywait(encargado->comDel->semCobrarPedidos);
    read(encargado->comDel->fifo,pedido,3);

    // Si el delivery le avisa que ya termino, cierra el local
    if(strcmp(pedido, ULTIMOPEDIDO))
      printf(NEGRO_T CYAN_F"\t\t$%.0f guardados de pedido %s"RESET_COLOR"\n", encargado->precios[atoi(pedido)], pedido);
    else {
      printf("\t\tCerrando local\n");
      * terminado = -1;
    }
  }else{
    printf(NEGRO_T BLANCO_F"\t\tNo hay pedidos por cobrar"RESET_COLOR"\n");
  }
}

// Hilo Telefono
void gestionTelefono(void * tmp){
  Telefono * telefono = (Telefono *) tmp;

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);

  // Ciclo de recibir llamadas, finaliza cuando suena la alarma
  while (timeout){
    usleep(rand() % 750001 + 250000);
    recibirLlamada(telefono);
  }

  // Envia el último pedido
  hacerUltimoPedido(telefono);

  // Cerramos el pipe
  close(telefono->tubo[0]);
  close(telefono->tubo[1]);
}

void recibirLlamada(Telefono * telefono) {

  // Entra pedido
  printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");
  hacerPedido(telefono);

  char temp [3];
  int error = 0;
  // El cliente esta 1 segundo esperando ser atendido
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 1;
  error = sem_timedwait(telefono->semaforoTelefono, &ts);
  if(error) {
    printf("\tSe perdio la llamada\n");
    read(telefono->tubo[0], temp, 3);
  }
}

void hacerPedido(Telefono * telefono) {
  char numeroPedido [2];

  snprintf(numeroPedido, 2, "%d", rand() % CARTA);
  usleep(rand()% 50001 + 50000); // Tiempo que tarda en pedir
  write(telefono->tubo[1], numeroPedido, 2);
}

void hacerUltimoPedido(Telefono * telefono) {

  // El telefono comienza a sonar
  printf(BLANCO_T ROJO_F"\tDueño llamando para cerrar local"RESET_COLOR"\n");
  // sem_post(telefono->semaforoLlamadas);
  sem_wait(telefono->semaforoTelefono);

  // Envia el ultimo pedido
  write(telefono->tubo[1], ULTIMOPEDIDO, 3);
  printf("Telefono_ultimoPedido_entregado()\n");
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
    if(strcmp(pedido, ULTIMOPEDIDO)) {
      usleep(rand()% 500001 + 1000000); // Tiempo que se demora en cocinar
      pedidoListo(cocinero, pedido);
    }
    else{
      cocinero->cantCocineros--;
      * terminado = -1;
      // El ultimo cocinero es el encargado de avisar a los deliveries que ya cerró la cocina
      if(cocinero->cantCocineros == 0) {
        pedidoListo(cocinero, ULTIMOPEDIDO);
      }
    }
  }
}

void pedidoListo(Cocinero * cocinero, char * pedido) {
  // Deja el pedido en el mostrador
  if(strcmp(pedido, ULTIMOPEDIDO)) {
    int enviado=mq_send(cocinero->enviar,pedido,strlen(pedido)+1,0);
    if (enviado == -1)
      perror("COCINERO mq_send");
  }
  else {
    for (int i = 0; i < DELIVERIES; i++){
      // Si el pedido actual es el ultimo, avisa a cada delivery que se vayan
      int enviado = mq_send(cocinero->enviar,pedido,strlen(pedido)+1,0);
      if (enviado == -1)
        perror("COCINERO mq_send");
      else
        printf("Cocinero_ultimoPedido_entregado()\n");
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
  delivery->comDel->fifo = open("/tmp/deliveryEncargado",O_WRONLY);
  if (delivery->comDel->fifo < 0) {
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
    if(strcmp(pedido, ULTIMOPEDIDO)) {
      usleep(rand()% 500001 + 500000);  // Tiempo que se demora en repartir pedido
      avisarCobro(delivery, pedido);
    }
     else {
      delivery->cantDelivery--;
      * terminado = -1;
      // Si es el ultimo delivery que termina, le avisa al encargado
      if(delivery->cantDelivery == 0)
        avisarCobro(delivery, ULTIMOPEDIDO);
    }
  }
}

void avisarCobro(Delivery * delivery, char * pedidoCobrar){

  // Deja el dinero
  if(strcmp(pedidoCobrar, ULTIMOPEDIDO))
    printf(NEGRO_T AMARILLO_F"\t\t\t\tPedido %s listo para cobrar"RESET_COLOR"\n", pedidoCobrar);
  // Avisa que se va
  else {
    printf("Delivery_ultimoPedido_entregado()\n");
    printf(BLANCO_T VERDE_F"\t\t\t\tPresione d para cerrar el local"RESET_COLOR"\n");
  }

  write(delivery->comDel->fifo, pedidoCobrar, 3);

  // Permite que el encargado pueda comenzar a cobrar
  sem_post(delivery->comDel->semCobrarPedidos);
}

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de comunicacion entre encargado y delivery
comDel * crearComDel() {
  comDel * com = (comDel *)(calloc(1, sizeof(comDel)));
  com->semCobrarPedidos = crearSemaforo("semCobrarPedidos", "comDel", 0);
  com->fifo = crearFIFO("deliveryEncargado");
  return com;
}

// Creacion de Telefono
Telefono * crearTelefono() {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = crearSemaforo("semTelefono", "tel", 0);
  printf("-----------------------------------------------------\n\n");
  telefono->tubo = (int *)(calloc(2, sizeof(int)));
  pipe(telefono->tubo);
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(Telefono * tel, mqd_t cola, comDel * com) {
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->comTel = tel;
  encargado->enviar = cola;
  encargado->comDel = com;
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
Delivery  * crearDelivery(mqd_t recibir, comDel * com) {
  Delivery * delivery = (Delivery *)(calloc(1, sizeof(Delivery)));
  delivery->recibir = recibir;
  delivery->comDel = com;
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
