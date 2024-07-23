#include <sys/wait.h> // waidpid()
#include <unistd.h> // chdir() fork() exec() pid_t()
#include <stdlib.h> // malloc() realloc() free() exit() execvp() EXIT_SUCCESS EXIT_FAILURE
#include <stdio.h> // fprintf() printf() stderr getchar() perror()
#include <string.h> // strcmp() strtok()

#include "myshell_parser.h" // pipeline_build(*command_line) pipeline_free(*pipeline)
#define TRUE 1 

char * type_prompt () {
	int bufsize = MAX_LINE_LENGTH + 1;
	char *buffer = malloc(sizeof(char)*bufsize);
	char *gets(char *buffer);
	//char * commandline;
	printf("my_shell $ ");
	
	if (!fgets(buffer, bufsize, stdin) && ferror(stdin))
		exit(EXIT_FAILURE);
	return buffer;
}

struct pipeline *read_command(char *command_line) {
	const char *c = command_line;
	struct pipeline *pipe = pipeline_build(c);
	free(command_line);
//	pipeline_print(pipe);
	return pipe;
}
int main (void) {
	// read - eval - print - loop
	while (TRUE) {
		char *command_line = type_prompt();
			read_command(command_line);
		
	//	if(fork()!=0) {
	///		/* parend conde 
//			waitpid(-1,&status, 0);
	//	} else {
			/*child code*/
	//		execve(command,parameters,0);
	//	}


	}

}
