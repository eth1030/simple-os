#ifndef myshell  /* Include guard */
#define myshell
#include <sys/wait.h> // waidpid()
#include <unistd.h> // chdir() fork() exec() pid_t()
#include <stdlib.h> // malloc() realloc() free() exit() execvp() EXIT_SUCCESS EXIT_FAILURE
#include <stdio.h> // fprintf() printf() stderr getchar() perror()
#include <string.h> // strcmp() strtok()

#include "myshell_parser.h"
#include "prinfunc.h"

char * type_prompt ();
struct pipeline *read_command(char *);
int runpipe(struct pipeline *);
int run(struct pipeline *);
int main();

#endif
