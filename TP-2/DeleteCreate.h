#ifndef _DELETECREATE_H_
#define _DELETECREATE_H_

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <mqueue.h>
#include <semaphore.h>

sem_t * crearSemaforo(char *, int);
void borrarSemaforo(sem_t *, char *);
mqd_t crearColaMensaje(char *, int, int, char *);
void borrarColaMensaje(mqd_t, char *, int);

#endif