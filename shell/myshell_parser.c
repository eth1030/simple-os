#include "myshell_parser.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#define AND "&"
#define PIPE "|"
#define IN "<"
#define OUT ">"
struct pipeline *pipeline_build(const char *command_line)
{ 	
	char * tokens = " \t\r\n\v<>|&";
	char *args[100];


	int countdelimeter = 1;
	int end; // end of word+token
	char string[MAX_LINE_LENGTH]; // copy of cmd line
	char check[MAX_LINE_LENGTH]; // copy of cmd line for checking
	strcpy(string,command_line);
	strcpy(check,command_line);


	char *save; // save ptr for strtok_r
	int mallocarray = 0;
	char *tk = strtok_r(string, tokens, &save);
	if (tk == NULL) 
		return NULL;
	args[mallocarray] = malloc(MAX_LINE_LENGTH*sizeof(char));
	strcpy(args[mallocarray],tk);
	// mallocarray++;

	int addlen; //length added to keep track of posit

	while (tk != NULL) {
		addlen = 0;
		mallocarray++;
		tk = strtok_r(NULL, tokens, &save);
		if (tk==NULL) {
			break;
		}
		addlen +=  strlen(tk) + 1;	
		args[mallocarray] = malloc(MAX_LINE_LENGTH*sizeof(char));
		strcpy(args[mallocarray],tk); 	


		end += addlen;




		countdelimeter++;
	}
	
	int aray1[(strlen(command_line))];
	for (int i = 0; i < (strlen(command_line)); i++) {
		switch(check[i]) {
					case'\t':
						aray1[i] = 1;
						break;
					case '\r':
						aray1[i] = 1;
						break;
					case '\n':
						aray1[i] = 1;
						break;
					case '\v':
						aray1[i] = 1;
						break;
					case ' ':
						aray1[i] = 1;
						break;
					case '<':
						aray1[i] = 1;
						break;
					case '>':
						aray1[i] = 1;
						break;
					case '|':
						aray1[i] = 1;
						break;
					case '&':
						aray1[i] = 1;
						break;
					default:
						aray1[i] = 0;
		}
	}




	bool casebackground = false;
	int cases;
	char casearray[100];
	casearray[0] = 0;
	int start = 0; int ending = 0; int casearraycnt = 0; int i = 0;
	while (i < (strlen(command_line))) {
		if (aray1[i] == 1) {
			start = i;
			ending = start;
			i++;
			while (aray1[i] == 1) { // here conditional jump or move depends on casearray
				ending++;
				i++;
			}

			if (start == ending) {

				switch(command_line[start]) {
					case '|':
						cases = 1;
	
					break;
					case '&':
						casebackground = true;

					break;
					case '>':
						cases = 3;

					break;
					case '<':
						cases = 4;

					break;
					default:
						cases = 0;

				}
			}
			else if (start < ending) {
				// loop from start to end
				// if special command is found - next arg will depend on it
				// else it is another command argument
				cases = 0;
				for (int iterator = start; iterator <= ending; iterator++) {
					switch(command_line[iterator]) {
						case '|':
							cases = 1;

						break;
						case '&':
							casebackground = true;

						break;
						case '>':
							cases = 3;

						break;
						case '<':
							cases = 4;

						break;
					}
					
				}
			}
			
			if (start != 0) {
				casearraycnt++;
				casearray[casearraycnt] = cases;
			}
				
		}
		else {
			i++;
		}
	}
	// allocate space for pipeline
	int count = 1;
	struct pipeline_command * ptr;
	struct pipeline *pipeline = malloc(sizeof *pipeline);
	pipeline->commands = malloc(sizeof(struct pipeline_command)); 
	ptr = pipeline->commands;
	pipeline->is_background = false;
	ptr->redirect_out_path = NULL;
	ptr->redirect_in_path = NULL;
	ptr->command_args[0] = args[0];
	ptr->command_args[1] = NULL;

	if (casebackground == true) {
		pipeline->is_background = true;
	}
	// allocate space for pipeline other than the initial pipeline
	for (int i = 1; i < countdelimeter;i++) {
		
		switch (casearray[i]) {

		case 0:
			;
			char *word = malloc(sizeof(args[i]));
			strcpy(word, args[i]);
			ptr->command_args[count] = word;
			count++;

			ptr->command_args[count] = NULL;

		break;

		case 1:
			;
			struct pipeline_command *nextcmd = malloc(sizeof(struct pipeline_command));
			ptr->next = nextcmd;
			ptr = nextcmd;

			char *word3 = malloc(sizeof(args[i]));
			strcpy(word3, args[i]);
			count = 0;
			ptr->command_args[count] = word3;
			count++;
			ptr->command_args[count] = NULL;
		break;

		case 2:
			// pipeline->is_background = true;

		break;

		case 3:
			;
			char *word1 = malloc(sizeof(args[i]));
			strcpy(word1, args[i]);
			ptr->redirect_out_path = word1;

		break;

		case 4:
		;
			char *word2 = malloc(sizeof(args[i]));
			strcpy(word2, args[i]);
			ptr->redirect_in_path = word2;

		break;
		}


		ptr->next = NULL;
		}
	return pipeline;
}

void pipeline_free(struct pipeline *p)
{
	struct pipeline_command* it = p->commands;
	//struct pipeline_command* itn;
		while (it != NULL ) {  // here
			int i = 0;
			while (it->command_args[i]!= NULL) { //here
				free(it->command_args[i]);
				i++;
			}
			free(it->redirect_in_path);  //here
			free(it->redirect_out_path); //here
			struct pipeline_command *itn = it->next; //here
			free(it);
			it = itn;
		}
		free(p);
}