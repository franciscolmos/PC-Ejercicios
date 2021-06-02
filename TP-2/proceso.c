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
  sem_t * semaforoLlamadas;
  int pedido;
  int * puntuacion;
  int * tubo;
}Telefono;

// ESTRUCTURA DEL ENCARGADO
typedef struct {
  Telefono * telefono;
  int comandaEnMano;
  int pedidoActual;
  float precios[CARTA];
}Encargado;

Telefono * crearTelefono(int *);
Encargado * crearEncargado(Telefono *);

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
void * gestionTelefono( void *);
void recibirLlamada(Telefono * telefono);
void TimeOut();

// FUNCIONES DEL ENCARGADO
void atenderPedido(Encargado *);

int LeerTeclado      (int salida);
int EscribirPantalla (int entrada);

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
      printf("proximamente\n");
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
  int puntuacion = 0; // Guarda la puntuacion del juego

  // Creamos los pipelines entre tel-enc. Tmb los semaforos

  // Creamos las colas de mensajes para enc-coc y coc-del

  // Creamos una fifo para del-enc. Tmb los semaforos


  // Se crean los actores del juego donde pasamos lo que creamos recien
  Telefono * telefono = crearTelefono(&puntuacion);
  Encargado * encargado = crearEncargado(telefono);
  // Cocinero * cocinero = crearCocinero(monitorComandas, monitorPedidos);
  // Delivery * delivery = crearDelivery(monitorPedidos, memoria);

  pid_t pid;

  pid = fork();
  if(pid == 0)
      gestionTelefono(telefono);
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
      close(encargado->telefono->tubo[1]);
      jugar(encargado);
      close(encargado->telefono->tubo[0]);
  }
  
  // Se liberan los semaforos
  // borrarSemMem(encargado, memoria);

  // Se libera la memoria de los objetos creados
  // free(telefono);
  // free(encargado);
  // free(cocinero);
  // free(delivery);

  // return puntuacion;

  return puntuacion;
}

// Hilo Encargado
void atenderPedido(Encargado * encargado) {
  // Si tiene una comanda en la mano, no puede atender el telefono
  if(encargado->comandaEnMano) {
    printf(NEGRO_T BLANCO_F"\t\tdejar comanda antes de atender otro pedido"RESET_COLOR"\n");
    return;
  }
  
  // Verifica si hay alguna llamada entrante
  int error = sem_trywait(encargado->telefono->semaforoLlamadas);
  if(!error) {
    int * tubo = encargado->telefono->tubo;
    char numeroPedido = '9';
    printf("\t\ttelefono atendido\n");
    usleep(rand()% 100001 + 250000); // Tiempo que tarda en tomar el pedido
    read(tubo[0],numeroPedido,2);
    encargado->pedidoActual = numeroPedido - '0';
    encargado->comandaEnMano = 1; // Flag, encargado debe cargar el pedido antes de atender otro

    if(encargado->pedidoActual != -1)
      printf(BLANCO_T MAGENTA_F"\t\tcomanda de pedido %d lista para cargar"RESET_COLOR"\n", 
            encargado->pedidoActual);
    else
      printf(BLANCO_T MAGENTA_F"\t\tavisar a los cocineros que cierren la cocina"RESET_COLOR"\n");

    // Cuelga el telefono
    sem_post(encargado->telefono->semaforoTelefono);
  }
}

// Hilo telefono
void * gestionTelefono(void * tmp){
  char numeroPedido = "";
  Telefono * telefono = (Telefono *) tmp;

  // Seteamos la alarma del juego e iniciamos el contador
  signal(SIGALRM, TimeOut);
  alarm(ALARMA);

  // Ciclo de recibir llamadas, finaliza cuando suena la alarma
  close(telefono->tubo[0]);
  while (timeout)
    recibirLlamada(telefono);

  // Envia el último pedido
  sem_wait(telefono->semaforoTelefono);
  write(telefono->tubo[1], menosUno(), 2);
  printf(BLANCO_T ROJO_F"\tDueño llamando para cerrar local"RESET_COLOR"\n");
  sem_post(telefono->semaforoLlamadas);

  close(telefono->tubo[1]);
}

void recibirLlamada(Telefono * telefono) {
  int error = 0;
  char numeroPedido = '9';
  // El telefono comienza a recibir llamadas mientras esta colgado
  sem_wait(telefono->semaforoTelefono);
  usleep(rand()% 750001 + 250000);
  telefono->pedido = rand() % CARTA;
  printf(BLANCO_T ROJO_F"\ttelefono sonando"RESET_COLOR"\n");

  // El telefono comienza a sonar
  sem_post(telefono->semaforoLlamadas);
  sleep(TIEMPOLLAMADA); // Tiempo que el cliente espera antes de cortar
  int telefonoSonando = 0;

  // Si el telefono no es atendido, se pierde la llamada
  sem_getvalue(telefono->semaforoLlamadas, &telefonoSonando);
  if(telefonoSonando > 0) {
    int error = sem_trywait(telefono->semaforoLlamadas);
    if(!error) {
      printf("\tSe perdio la llamada\n");
      sem_post(telefono->semaforoTelefono);
    }
  }
  else
    sprintf(numeroPedido, "%c", telefono->pedido + 48);
    error=write(telefono->tubo[1], numeroPedido, 2);
    *telefono->puntuacion = *telefono->puntuacion+1; // Si se atiende, suma un punto
}

void TimeOut() {
  timeout = 0;
}

Telefono * crearTelefono(int * puntuacion) {
  Telefono * telefono = (Telefono *)(calloc(1, sizeof(Telefono)));
  telefono->semaforoTelefono = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoTelefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 1);
  telefono->semaforoLlamadas = (sem_t *)(calloc(1, sizeof(sem_t)));
  telefono->semaforoLlamadas = sem_open("/semLlamadas", O_CREAT, O_RDWR, 0);
  telefono->puntuacion = puntuacion;
  telefono->tubo = (int *)(calloc(2, sizeof(int)));
  int error = 0;
  error = pipe(telefono->tubo);
  if(error)
    perror("error al crear el pipe");
  return telefono;
}

// Creacion de Encargado
Encargado * crearEncargado(Telefono *telefono){
  Encargado * encargado = (Encargado *)(calloc(1, sizeof(Encargado)));
  encargado->telefono = telefono;
  for(int i = 0; i < CARTA; i++)
    encargado->precios[i] = 100 * (i+1);
  return encargado;
}

char * menosUno(){
  int number1 = 45;
  int number2 = 49;
  char signo [2];
  char charNumber [2];
  sprintf(signo, "%c", number1);
  sprintf(charNumber,"%c" , number2);
  char string [1];
  strcpy(string,signo);
  strcat(string, charNumber);
  return string;
}

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