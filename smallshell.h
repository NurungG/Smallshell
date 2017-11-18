#ifndef __SMALLSHELL_H__
#define __SMALLSHELL_H__

/* Include */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>


/* Define */
#define EOL 1
#define ARG 2
#define AMPERSAND 3
#define SEMICOLON 4
#define PIPELINE 5
#define LESS 6
#define GREATER 7
#define INTERRUPT 8

#define MAXARG 512
#define MAXBUF 512

#define FOREGROUND 0
#define BACKGROUND 1

/* Signal handlers */
void handle_int(int);

/* Public Functions */
int userin();
void procline();


#endif
