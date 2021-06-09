#include "DeleteCreate.h"

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

mqd_t crearColaMensaje(char * nomCola, int create, int tipoApertura, char * identificador){
   mqd_t temp;
   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = 10;
   attr.mq_msgsize = 4;
   attr.mq_curmsgs = 0;
   char nomColaConBarra [50] = "/";
   strcat(nomColaConBarra, nomCola);
   char open [50] = "mq_open_";
   char openF [50];
   strcat(open, nomCola);
   strcat(open, identificador);
   strcpy(openF, open);
   strcat(openF, identificador);
   strcat(open, "_ok()");
   strcat(openF, "_failed()");

   if(create){
      switch (tipoApertura)
      {
      case 1:
         temp = mq_open(nomColaConBarra , O_WRONLY | O_CREAT, 0777, &attr);
         if(temp == -1)
            perror(openF);
         else
            printf("%s\n",open);
         break;
      case 0:
         temp = mq_open(nomColaConBarra , O_RDONLY | O_CREAT, 0777, &attr);
         if(temp == -1)
            perror(openF);
         else
            printf("%s",open);
         break;
      default:
         break;
      }
   }else{
       switch (tipoApertura)
      {
      case 1:
         temp = mq_open(nomColaConBarra , O_WRONLY, 0777, &attr);
         if(temp == -1)
            perror(openF);
         else
            printf("%s",open);
         break;
      case 0:
         temp = mq_open(nomColaConBarra , O_RDONLY, 0777, &attr);
         if(temp == -1)
            perror(openF);
         else
            printf("%s",open);
         break;
      default:
         break;
      }
   }
   return temp;
}

void borrarColaMensaje(mqd_t cola, char * nomCola, int unlink){
   char nomColaConBarra [50] = "/";
   strcat(nomColaConBarra, nomCola);
   char close [50] = "mq_close_";
   char closeF [50];
   strcat(close, nomCola);
   strcpy(closeF, close);
   strcat(close, "_ok()");
   strcat(closeF, "_failed()");

   if(unlink){
      char unlink [50] = "mq_unlink_";
      char unlinkF [50];
      strcat(unlink, nomCola);
      strcpy(unlinkF, unlink);
      strcat(unlink, "_ok()");
      strcat(unlinkF, "_failed()");
      int status = mq_close(cola);
      if(!status) {
         printf("%s\n", close);
         status = mq_unlink(nomColaConBarra);
            if(!status)
               printf("%s\n", unlink);
            else
               perror(unlinkF);
      }
      else
         perror(closeF);
   }else{
      int status = mq_close(cola);
      if(!status)
         printf("%s\n", close);
      else
         perror(closeF);
   }
}