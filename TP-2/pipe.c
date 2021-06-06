#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

int main(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   printf("%d\n", ts.tv_sec);
   ts.tv_sec += 1;
   printf("%d\n", ts.tv_sec);
   return 0;
}

