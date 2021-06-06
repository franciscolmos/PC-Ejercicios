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
#define TIEMPOLLAMADA 1
#define ULTIMOPEDIDO -1

int timeout = 1; // INDICA CUANDO EL JUEGO VA A TERMINAR

// ESTRUCTURA DEL TELEFONO
typedef struct {
  sem_t * semaforoTelefono;
  int pedido;
  int * tubo;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  sem_t * semaforoTelefono;
  int * tubo;
  int puntuacion;
  int comandaEnMano;
  int pedidoActual;
  float precios[CARTA];
}Encargado;

Telefono * crearTelefono(sem_t *, int *);
Encargado * crearEncargado(sem_t *, int *);

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
      printf("\t\tComanda entregada\n");
      encargado->comandaEnMano = 0;
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
  semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 0);
  int * tubo = (int *)(calloc(2, sizeof(int)));
  int error = pipe(tubo);
  if(error)
    perror("error al crear el pipe");

  // Creamos las colas de mensajes para enc-coc y coc-del

  // Creamos una fifo para del-enc. Tmb los semaforos


  // Se crean los actores del juego donde pasamos lo que creamos recien
  Telefono * telefono = crearTelefono(semaforoTelefono, tubo);
  Encargado * encargado = crearEncargado(semaforoTelefono, tubo);
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
  /* else if(pid > 0) {
      pid = fork();
      if(pid == 0)
          printf("Soy el hijo numero 2. Los cocineros\n");
      else if(pid > 0) {
          pid = fork();
          if(pid == 0)
              printf("Soy el hijo numero 3. Los deliverys\n");
      } 
  }*/

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
  sem_post(encargado->semaforoTelefono);
  int telefonoSonando = 0;
  usleep(100000);
  sem_getvalue(encargado->semaforoTelefono, &telefonoSonando);
  if(telefonoSonando == 0){
    char numeroPedido[3];
    usleep(rand()% 100001 + 100000); // Tiempo que tarda en tomar el pedido
    read(encargado->tubo[0], numeroPedido, 3);
    encargado->pedidoActual = atoi(numeroPedido);
    encargado->comandaEnMano = 1; // Flag, encargado debe cargar el pedido antes de atender otro

    if(encargado->pedidoActual != -1) {
      printf(BLANCO_T MAGENTA_F"\t\tcomanda de pedido %d lista para cargar"RESET_COLOR"\n", 
            encargado->pedidoActual);
      encargado->puntuacion++;
    }
    else {
      printf(BLANCO_T MAGENTA_F"\t\tavisar a los cocineros que cierren la cocina"RESET_COLOR"\n");
      *terminado = -1;
    }
  }
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
}

void recibirLlamada(Telefono * telefono) {
  printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");
  int error = 0;
  char numeroPedido [2];
  // El telefono comienza a recibir llamadas mientras esta colgado
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 1;
  error = sem_timedwait(telefono->semaforoTelefono, &ts);
  if(!error){
    printf("\ttelefono atendido\n");
    telefono->pedido = rand() % CARTA;
    snprintf(numeroPedido, 2, "%d", telefono->pedido);
    error=write(telefono->tubo[1], numeroPedido, 2);
    usleep(200000);
  }else{
    printf("\tSe perdio la llamada\n");
  }
}

void TimeOut() {
  timeout = 0;
}

/*----------------------------------------------------------------------------*/
/*-----------------------FUNCIONES DE INICIALIZACION--------------------------*/
// Creacion de Telefono
Telefono * crearTelefono(sem_t * semaforo, int * pipe) {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = semaforo;
  telefono->tubo = pipe;
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(sem_t * semaforo, int * pipe) {
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->semaforoTelefono = semaforo;
  encargado->tubo = pipe;
  for(int i = 0; i < CARTA; i++)
    encargado->precios[i] = 100 * (i+1);
  return encargado;
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