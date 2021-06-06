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
#include <mqueue.h>

// COLORES
#define RESET_COLOR  "\x1b[0m"
#define NEGRO_T      "\x1b[30m"
#define ROJO_F       "\x1b[41m"
#define VERDE_F      "\x1b[42m"
#define AMARILLO_F   "\x1b[43m"
#define MAGENTA_F    "\x1b[45m"
#define BLANCO_T     "\x1b[37m"
#define BLANCO_F     "\x1b[47m"
#define TAMMSG 8192

// DATOS DEL JUEGO
#define ENCARGADOS 1
#define COCINEROS 2
#define DELIVERIES 2
#define CARTA 5
#define ALARMA 10
#define TIEMPOLLAMADA 1
#define ULTIMOPEDIDO -1

int timeout = 1; // INDICA CUANDO EL JUEGO VA A TERMINAR

// ESTRUCTURA DEL TELEFONO
typedef struct {
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  int pedido;
  int * tubo;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  sem_t * semaforoTelefono;
  sem_t * semaforoLlamadas;
  mqd_t enviar;
  int puntuacion;
  int * tubo;
  int comandaEnMano;
  int pedidoActual;
  float precios[CARTA];
}Encargado;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  mqd_t recibir;
}Cocinero;

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// ACTORES
Telefono * crearTelefono(sem_t *, sem_t *, int *);
Encargado * crearEncargado(sem_t *, sem_t *, int *, mqd_t);
Cocinero * crearCocinero(mqd_t);

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
void recibirLlamada(Telefono * telefono);
void TimeOut();

// FUNCIONES DEL ENCARGADO
void atenderPedido(Encargado *, int *);
void cargarPedido(Encargado *, int *);

// FUNCIONES DEL COCINERO
void * gestionCocinero(void *);
void cocinarPedido(Cocinero *, int *);

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
      atenderPedido(encargado, terminado);
      break;
    case 's':
      cargarPedido(encargado, terminado);
      /* printf("\t\tComanda entregada\n");
      encargado->comandaEnMano = 0; */
      break;
    case 'd':
      printf("proximamente\n");
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
  // Creamos los pipelines entre tel-enc. Tmb los semaforos
  sem_t * semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  sem_t * semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  int * tubo = (int *)(calloc(2, sizeof(int)));
  int error = pipe(tubo);
  if(error)
    perror("error al crear el pipe");

  // Creamos las colas de mensajes para enc-coc y coc-del
  mqd_t nqdComandas;
  nqdComandas=mq_open("/encargadoCocineros",O_RDWR | O_CREAT, 0777, NULL);
  if (nqdComandas==-1) {
    perror("mq_open 1");
    error=nqdComandas;
  }


  // Creamos una fifo para del-enc. Tmb los semaforos


  // Se crean los actores del juego donde pasamos lo que creamos recien
  Telefono * telefono = crearTelefono(semaforoTelefono, semaforoLlamadas, tubo);
  Encargado * encargado = crearEncargado(semaforoTelefono, semaforoLlamadas, tubo, nqdComandas);
  Cocinero * cocinero = crearCocinero(nqdComandas);
  // Telefono * telefono = crearTelefono(&puntuacion);
  // Encargado * encargado = crearEncargado(telefono);
  // Cocinero * cocinero = crearCocinero(monitorComandas, monitorPedidos);
  // Delivery * delivery = crearDelivery(monitorPedidos, memoria);

  pid_t pid;

  pid = fork();
  if(pid == 0) {
      gestionTelefono(telefono);
      exit(0);
  }
    else if(pid > 0) {
      pid = fork();
      if(pid == 0){
          gestionCocinero(telefono);
          pthread_t hilosCocineros[COCINEROS];
          for(int i = 0; i < COCINEROS; i++){
            pthread_create(&hilosCocineros[i], NULL, gestionCocinero, (void *)(cocinero));
            pthread_join(hilosCocineros[i], NULL);
          }     
      }
      /* else if(pid > 0) {
          pid = fork();
          if(pid == 0)
              printf("Soy el hijo numero 3. Los deliverys\n");
      }  */
  }

  // Arranca el ciclo de juego del encargado, donde se detecta las teclas que presiona
  if(pid > 0){
      close(encargado->tubo[1]);
      jugar(encargado);
      close(encargado->tubo[0]);
  }
  
  // Se liberan los semaforos
  // borrarSemMem(encargado, memoria);

  int status=0;
  // Semaforo Telefono
  status = sem_close(encargado->semaforoTelefono);
  if (!status) {
    status = sem_unlink("/semTelefono");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  status = sem_close(encargado->semaforoLlamadas);
  if (!status) {
    status = sem_unlink("/semLlamadas");
    if (status)
      perror("sem_unlink()");
  }
  else
    perror("sem_close()");

  if (!access("./encargadoCocineros",F_OK)) {
    nqdComandas=mq_close(nqdComandas);
    if (nqdComandas) {
      perror("mq_close 1");
      error=nqdComandas;
    }
    nqdComandas=mq_unlink("./encargadoCocineros");
    if (nqdComandas) {
      perror("mq_close 1");
      error=nqdComandas;
    }
  }

  // Se libera la memoria de los objetos creados
  // free(telefono);
  // free(encargado);
  // free(cocinero);
  // free(delivery);

  return encargado->puntuacion;
}

// Hilo Encargado
void atenderPedido(Encargado * encargado, int * terminado) {
  // Si tiene una comanda en la mano, no puede atender el telefono
  if(encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tdejar comanda antes de atender otro pedido"RESET_COLOR"\n");
    return;
  }
  // Verifica si hay alguna llamada entrante
  int error = sem_trywait(encargado->semaforoLlamadas);
  if(!error) {
    printf("\t\ttelefono atendido\n");
    char numeroPedido[3];
    usleep(rand()% 100001 + 100000); // Tiempo que tarda en tomar el pedido
    read(encargado->tubo[0], numeroPedido, 3);
    encargado->pedidoActual = atoi(numeroPedido);
    encargado->comandaEnMano = 1; // Flag, encargado debe cargar el pedido antes de atender otro

    if(encargado->pedidoActual != -1){
        printf(BLANCO_T MAGENTA_F"\t\tcomanda de pedido %d lista para cargar"RESET_COLOR"\n", 
            encargado->pedidoActual);
      encargado->puntuacion++;
    }
    else{
      printf(BLANCO_T MAGENTA_F"\t\tavisar a los cocineros que cierren la cocina"RESET_COLOR"\n");
      *terminado = -1;
    }

    // Cuelga el telefono
    sem_post(encargado->semaforoTelefono);
  }
}

void cargarPedido (Encargado * encargado, int * terminado) {
  char pedido[3];
  // Si no tiene una comanda en la mano, no puede cargar ningun pedido
  if(!encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tatender el telefono antes de cargar un pedido"RESET_COLOR"\n");
    return;
  }
  if(encargado->pedidoActual != ULTIMOPEDIDO){
    snprintf(pedido,3,"%d",encargado->pedidoActual);
    int enviado=mq_send(encargado->enviar,pedido,3,0);
    if (enviado==-1) {
      perror("ENCARGADO mq_send");
    }
  }else{
      for (int i = 0; i < COCINEROS-1; i++){
        // Si el pedido actual es el ultimo, avisa a cada cocinero que cierren la cocina
        snprintf(pedido,3,"%d",encargado->pedidoActual);
        int enviado=mq_send(encargado->enviar,pedido,3,0);
        if (enviado==-1) {
            perror("ENCARGADO mq_send");
        }
      }
  }
  encargado->comandaEnMano = 0;
}

// Hilo telefono
void gestionTelefono(void * tmp){
  Telefono * telefono = (Telefono *) tmp;

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);

  // Ciclo de recibir llamadas, finaliza cuando suena la alarma
  close(telefono->tubo[0]);
  while (timeout){
    usleep(rand() % 500000 + 250000);
    recibirLlamada(telefono);
  }
  // Envia el último pedido
  printf(BLANCO_T ROJO_F"\tDueño llamando para cerrar local"RESET_COLOR"\n");
  sem_wait(telefono->semaforoTelefono);
  char cadena[3];
  snprintf(cadena, 3, "%d", ULTIMOPEDIDO);
  write(telefono->tubo[1], cadena, 3);
  close(telefono->tubo[1]);
  sem_post(telefono->semaforoLlamadas);
}

void recibirLlamada(Telefono * telefono) {
  int error = 0;
  char numeroPedido [2];

  // El telefono comienza a recibir llamadas mientras esta colgado
  sem_wait(telefono->semaforoTelefono);
  usleep(rand() % 750001 + 250000);

  // El telefono comienza a sonar
  printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");
  sem_post(telefono->semaforoLlamadas);
  sleep(TIEMPOLLAMADA); // Tiempo que el cliente espera antes de cortar
  int telefonoSonando = 0;

  sem_getvalue(telefono->semaforoLlamadas, &telefonoSonando);
  if(telefonoSonando > 0) {
    error = sem_trywait(telefono->semaforoLlamadas);
    if(!error) {
      printf("\tSe perdio la llamada\n");
      sem_post(telefono->semaforoTelefono);
    }
  }else{
    telefono->pedido = rand() % CARTA;
    snprintf(numeroPedido, 2, "%d", telefono->pedido);
    error=write(telefono->tubo[1], numeroPedido, 2);
  }
}

void TimeOut() {
  timeout = 0;
}

// Hilo Cocinero
void * gestionCocinero(void * tmp) {

  Cocinero *cocinero = (Cocinero *) tmp;
  int * terminado = (int *)(calloc(1, sizeof(int)));

  // Ciclo de cocinar pedidos, finaliza cuando el encargado avisa
  while(*terminado != -1)
    cocinarPedido(cocinero, terminado);

  free(terminado);
  pthread_exit(NULL);
}

void cocinarPedido(Cocinero * cocinero, int * terminado) {

  // Toma una comanda para empezar a cocinar
  char pedido[3];
  int recibido=mq_receive(cocinero->recibir,pedido,3,0);
    if (recibido == -1) {
      perror("COCINERO mq_receive");
    }
    else {
        printf("\t\t\tPedido %s cocinado", pedido);
    }
}

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de Telefono
Telefono * crearTelefono(sem_t * semaforoTelefono, sem_t * semaforoLlamadas, int * pipe) {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = semaforoTelefono;
  telefono->semaforoLlamadas = semaforoLlamadas;
  telefono->tubo = pipe;
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(sem_t * semaforoTelefono, sem_t * semaforoLlamadas, int * pipe, mqd_t enviar) {
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->semaforoTelefono = semaforoTelefono;
  encargado->semaforoLlamadas = semaforoLlamadas;
  encargado->tubo = pipe;
  encargado->enviar = enviar;
  for(int i = 0; i < CARTA; i++)
    encargado->precios[i] = 100 * (i+1);
  return encargado;
}

// Creacion de Cocinero
Cocinero * crearCocinero(mqd_t recibir) {
  Cocinero * cocinero = (Cocinero *)(calloc(1, sizeof(Cocinero)));
  cocinero->recibir = recibir;
  return cocinero;
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