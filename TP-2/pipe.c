#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <mqueue.h>
#include <semaphore.h>

sem_t * crearSemaforo(char *, int);
void borrarSemaforo(sem_t *, char *);

int main(void) {
   char nomSemaforo [] = "sem1";
   sem_t * semaforo = crearSemaforo(nomSemaforo, 1);
   borrarSemaforo(semaforo, nomSemaforo);
   return 1;
}

sem_t * crearSemaforo(char * nomSemaforo, int valorInicio) {

   char open [50] = "sem_open_";
   char openF [50];
   strcat(open, nomSemaforo);
   strcpy(openF, open);
   strcat(open, "_ok()");
   strcat(openF, "_failed()");

   sem_t * semaforo = (sem_t *)(calloc(1, sizeof(sem_t)));
   semaforo = sem_open(nomSemaforo, O_CREAT, O_RDWR, valorInicio);
   if(semaforo == SEM_FAILED)
      perror(openF);
   else
      printf("%s\n", open);

   return semaforo;
}

void borrarSemaforo(sem_t * semaforo, char * nomSemaforo) {
   char close [50] = "sem_close_";
   char closeF [50];
   strcat(close, nomSemaforo);
   strcpy(closeF, close);
   strcat(close, "_ok()");
   strcat(closeF, "_failed()");

   char unlink [50] = "sem_unlink_";
   char unlinkF [50];
   strcat(unlink, nomSemaforo);
   strcpy(unlinkF, unlink);
   strcat(unlink, "_ok()");
   strcat(unlinkF, "_failed()");

   int status = sem_close(semaforo);
   if(!status) {
      printf("%s\n", close);
      status = sem_unlink(nomSemaforo);
         if(!status)
            printf("%s\n", unlink);
         else
            perror(unlinkF);
   }
   else
      perror(closeF);
}