#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

int main(void)
{
   char cadena [50];
   strncpy(cadena, "OLA K ASE", 50);
   printf("CADENA: %s\nTAMAÑO: %d\n", cadena, strlen(cadena));
   strncpy(cadena, "OLAAAAA K ASEEEE", 50);
   printf("CADENA: %s\nTAMAÑO: %d\n", cadena, strlen(cadena));
}
