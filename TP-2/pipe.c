#include<stdio.h>
#include<string.h>

int main(void)
{
    char cadena[3];
    snprintf(cadena,3,"%d", -1);
    printf("%s",cadena);

    return 0;
}

