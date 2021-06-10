#ifndef _DELETECREATE_H_
#define _DELETECREATE_H_

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <mqueue.h>
#include <semaphore.h>

sem_t * crearSemaforo(char *, char *, int);
mqd_t crearColaMensaje(char *, char *, int);
int cerrarSemaforo(sem_t *, char *, char *);
void borrarSemaforo(sem_t *, char *, char *);
int cerrarColaMensaje(mqd_t, char *, char *);
void borrarColaMensaje(mqd_t, char *, char *);

#endif