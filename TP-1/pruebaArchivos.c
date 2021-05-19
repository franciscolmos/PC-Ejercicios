#include <stdio.h>
#include <stdlib.h>

int main() {

    FILE * archivoPuntuacion;
    archivoPuntuacion = fopen("./puntuacion.txt", "w");
    if(archivoPuntuacion == NULL) {
        perror("fopen()");
        return 1;
    }
    fprintf(archivoPuntuacion, "test");
    fflush(archivoPuntuacion);
    fclose(archivoPuntuacion);
    return 0;
}