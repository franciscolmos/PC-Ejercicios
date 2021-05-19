#ifndef _CONIO_H_
#define _CONIO_H_
#include <termios.h>
#include <stdio.h>

static struct termios old, current;
void initTermios(int);
void resetTermios(void);
char getch_(int);
char getch(void);
char getche(void);

#endif
