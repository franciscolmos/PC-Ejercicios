#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> /* POSIX -> gcc -pthread */

void *buscarMayor(void *);

typedef struct {
  int mayor;
  int posMayor;
  int nroHilo;
  int delta; // porcion de vector a buscar
  int * pointerC;
} Vector;

int main(){
    Vector vectorMayor;
    vectorMayor.mayor = 0;
    vectorMayor.posMayor = -1;
    vectorMayor.nroHilo = -1;

    srand(time(NULL));

    // INICIALIZAMOS EL VECTOR DE NUMEROS PARA BUSCAR EL MAYOR

    vectorMayor.pointerC = (int*)calloc(20000, sizeof(int));
    for( int i = 0; i < 20000; i++ ) {
        vectorMayor.pointerC[i] = rand() % 100000;
    }

    int cantHilos = 0;
    printf("Ingrese la cantidad de hilos que desee crear: ");
    scanf("%d", &cantHilos);

    vectorMayor.delta = 20000 / cantHilos;

    pthread_t hilos[cantHilos];
    for( int i = 0; i < cantHilos; i++ ){
        printf("hilo %d", i);
        pthread_create(&hilos[i], NULL, buscarMayor, &vectorMayor);
    }
    sleep(1);
    // while(cerosUnos.encontrado == 0) {

    // }

    return 0;
}

void *buscarMayor(void *tmp){
    Vector* p = (Vector *)(tmp);
    p->nroHilo++;
    printf("----NUMERO DE HILO: %d", p->nroHilo);
    int secuencia = p->nroHilo;
    for (int i = (secuencia * p->delta) ; i < ((secuencia + 1) * p->delta); i++)
    {
        if(p->pointerC[i] > p->mayor){
            p->mayor = p->pointerC[i];
            p->posMayor = i;
        }
    }
    printf("\ninicio %d, secuencia %d, nroHilo %d\n", (secuencia * p->delta), ((secuencia+1) * p->delta), (p->nroHilo * p->delta));
    pthread_exit(NULL);
}