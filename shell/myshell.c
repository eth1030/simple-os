#include <sys/wait.h> // waidpid()
#include <unistd.h> // chdir() fork() exec() pid_t()
#include <stdlib.h> // malloc() realloc() free() exit() execvp() EXIT_SUCCESS EXIT_FAILURE
#include <stdio.h> // fprintf() printf() stderr getchar() perror()
#include <string.h> // strcmp() strtok()
#include <sys/types.h>
#include <fcntl.h>

#include "myshell_parser.h" // pipeline_build(*command_line) pipeline_free(*pipeline)
#define TRUE 1 
#define FALSE 0
#define STD_INPUT 0
#define STD_OUTPUT 1
char * type_prompt (char * mode) {
	int bufsize = MAX_LINE_LENGTH + 1;

	char *buffer = malloc(sizeof(char)*bufsize);
	char *gets(char *buffer);
	if (mode != NULL) {
		if ((strcmp(mode, "-n"))!= 0)
			printf("my_shell$ ");
	}
	else
		printf("my_shell$ ");
	if(fgets(buffer,bufsize,stdin) == NULL)
		return NULL;
	fflush(NULL);
	return buffer;
}

struct pipeline *read_command(char *command_line) {
	const char *c = command_line;
	struct pipeline *p = pipeline_build(c);
	free(command_line);
	//	pipeline_print(pipe);
	return p;
}


void runpipe(struct pipeline *p) {
     int numpipe = 0; //int child_status;
        struct pipeline_command * it = p->commands;
		struct pipeline_command * current = p->commands;
        while (it!=NULL) {
                numpipe++;
                it = it->next;
        } // finds number of pipes
        int fd[numpipe -1][2]; 

		for (int i = 0; i < numpipe; i++) {
			if (pipe(fd[i]) < 0) {
				perror("ERROR");
				exit(EXIT_FAILURE);
			}
		}
	pid_t child_pid; int status = 0;
	for (int i = 0; i < numpipe; i++) {
		child_pid = fork();
		if (child_pid < 0) {
			perror("ERROR");
			exit(EXIT_FAILURE);
		}
		else if (child_pid == 0) {
			if (i==0) {
				close(fd[i][0]);
				dup2(fd[i][1],STD_OUTPUT);
				close(fd[i][1]);
				if (current->redirect_in_path) {
					if ((fd[i][0] = open(current->redirect_in_path,O_RDONLY)) == -1)
						perror("ERROR:");;
					dup2(fd[i][0], STD_INPUT);
					close(fd[i][0]);
				}

			}
			else if (current->next == NULL) {
				close(fd[i -1][1]);
				dup2(fd[i -1][0],STD_INPUT);
				close(fd[i -1][0]);
				if (current->redirect_out_path) {
					if((fd[i-1][0] = creat(current->redirect_out_path, 0644)) == -1)
						perror("ERROR:");
					dup2(fd[i-1][0], STD_OUTPUT);
					close(fd[i-1][0]);
				}
			}
			else {
				// close(fd[i-1][0]);
				close(fd[i -1 ][1]);
				dup2(fd[i-1][0],STD_INPUT);
				close(fd[i-1][0]);
				dup2(fd[i][1],STD_OUTPUT);
				close(fd[i][1]);
			}
			if (execvp(current->command_args[0],current->command_args) < 0 ) {
				perror("ERROR");
				exit(EXIT_FAILURE);
			}
		}
		else {
			if (i != 0) {
				close(fd[i-1][0]);
				close(fd[i-1][1]);
			}
			current = current->next;
		}
		
	}
	for (int j = 0; j < numpipe -1; j++) {
			close(fd[j][0]);
			close(fd[j][1]);
	}
	do {
		if(!(p->is_background)) {
			for(int k = 0; k < numpipe -1; k++)
				waitpid(-1,NULL,0);
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		
		
}

int run(struct pipeline *p) {
	int child_status;
	bool background = p->is_background;
	pid_t child_pid = fork();
	int readin;
	int write; 

	if(child_pid > 0 ) {
		//  parent
		do {
			if(!(background)) {
				waitpid(child_pid,&child_status,0);
			}
		} while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
		return child_pid;
	}
	else if (child_pid == 0) {
		// child

		if (p->commands->redirect_in_path && p->commands->redirect_out_path) {
			if ((readin = open(p->commands->redirect_in_path,O_RDONLY)) == -1)
				perror("ERROR:");
			if((write = creat(p->commands->redirect_out_path, 0644)) == -1)
				perror("ERROR:");
			dup2(write, STD_OUTPUT);
			dup2(readin, STD_INPUT);
		}
		else if (p->commands->redirect_in_path) {
			if ((readin = open(p->commands->redirect_in_path,O_RDONLY)) == -1)
				perror("ERROR:");
			dup2(readin, STD_INPUT);
		}
		else if (p->commands->redirect_out_path) {
			if((write = creat(p->commands->redirect_out_path, 0644)) == -1)
				perror("ERROR:");
			dup2(write, STD_OUTPUT);
		}
	
		if (execvp(p->commands->command_args[0],p->commands->command_args) == -1) {
				perror("ERROR:");
				exit(EXIT_FAILURE);
		}
		if(p->commands->redirect_in_path)
			close(readin);
		if(p->commands->redirect_out_path)
			close(write);
	}
	else if (child_pid < 0 ) {
		// fork failure
		perror("ERROR:");
	}
	return child_pid;
}

int main(int argc, char *argv[]) {
	// read - eval - print - loop
	while (TRUE) {
		// read cmd line input
		char *command_line = type_prompt(argv[1]);
		
		// exit when user enters control-D
		if ((command_line== NULL)) {
			free(command_line);
			exit(EXIT_SUCCESS);
			return 0;
		}
		else { // create a pipeline to run
			struct pipeline *p = read_command(command_line);
			if (p->commands->next == NULL)
				run(p);
			else
				runpipe(p);
			pipeline_free(p);
		}
	}

}
