#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

int main(void)
{
   char cadena [2];
   snprintf(cadena, 3, "%d", -1);
   if(!strcmp(cadena, "-1")){
      printf("EA!\n");
   }
}
