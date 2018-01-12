/*
Alec Parent - 260688035
COMP 310 - Operating Systems
Assignment 1 Part B - A Simple Shell

*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

//Define for loop in main. Just a structural design I prefer
#define TRUE 1

//Define a Linked List structure to store the background jobs
typedef struct ProcessList{
	pid_t pid;
	int number;
	char *args;
	struct ProcessList *next;
}ProcessList;

//Various global declarations to run the shell's various uses
ProcessList *List;
int backProcess = 0; //Number of background processes
int stdoutRedirect = 0; //A flag to signal the desire to redirect the output
int console; //A duplication of stdout
int noinput = 0; //A flag to signal that the input string was empty

char *command; //Manipuating command may alter it in some ways, so I store it here in a global value before manipulation
char *stdoutlocation; //Store name of redirection file in case of demand of output redirection

static void childWait();

//Function called whenever a new node is created and stored in the background
void insertbg(ProcessList *allProcesses, pid_t this_pid, char *this_args){
//Create the new node
	ProcessList *this = malloc(sizeof(ProcessList));
	this->pid = this_pid;
	this->args = this_args;
	this->next = NULL;
	this->number = 1;

//Link it to the full list
	ProcessList *curr = allProcesses;
	while(curr->next != NULL){
		curr = curr->next;
		this->number = this->number + 1;
	}
	curr->next = this;

//Increase number of background processes
	backProcess++;
}

//If an "fg" call removes a process from the list, change the numbers so that they are sequential
void reformat(){
	ProcessList *curr = List->next;
	int i = 1;
	while(curr != NULL){
		curr->number = i;
		i++;
	}
}

int getcmd(char *prompt, char *args[], int *bg){

	int length = 0; //The length of the input 
	int i = 0; //The return value, the number of arguments in the command

	char *token = NULL; //Tokenization of the input will take place in this variable
	char *loc = NULL; //A flag that will detect the '>' symbol for output redirection
	char *line = NULL; //where the input command will be stored
	
	size_t linecap = 0;

	command = NULL;

	printf("%s", prompt);
	length = getline(&line, &linecap, stdin);
	
//Exit the function immediately if no inputis detected
	if (length == 1){
		noinput = 1;
		return 0;
	}
	else{
		noinput = 0;
	}

//Store in a global variable the size of the input (explanation near global variables)
	command = malloc(sizeof(strlen(line)));
	strcpy(command, line);

//Detect demand for output redirection	
	loc = index(line, '>');
	if(loc != NULL){
		int index = (int)(loc-line); //The exact index of the occurrence of '>'
		stdoutRedirect = 1; //Set flag, as described above
	
//Split the strings in order to differentiate between command and redirection location
		stdoutlocation = malloc(sizeof(strlen(line+1)));
		strncpy(stdoutlocation, &line[index+2], sizeof(line) + 1);
		line = malloc(sizeof(strlen(command + 1)));
		strncpy(line, command, index - 1);
	}

//Detect if background is specified
	loc = index(line, '&');
	if(loc != NULL){
		*loc = ' ';
		*bg = 1;
//Remove '&' to add command to jobs list
		loc = index(command, '&');
		*loc = ' ';
	}
	else{
		*bg = 0;
		while((token = strsep(&line, " \t\n")) != NULL){
			for(int j = 0; j < strlen(token); j++){
				if (token[j] <= 32){ //Separate the string if a particular char is not a letter, number, punctuation, etc.
					token[j] = '\0';
				}
			}
			if(strlen(token) > 0){
				args[i] = token;
				i++;
			}
		}
	}

	return i;
}

//A function which serves to execute certain functions, and returns a status indicating how said command should be executed, i.e. determine if we're running from this shell's memory or if we're advancing to execvp
int builtin(char *args[], int cnt){
	int i = 0;
	int fd;
//Type "exit" to close the program.
	if(!strcmp(args[0], "exit")){
		childWait();
		printf("Exiting Shell.\n");
		exit(EXIT_SUCCESS);
	}
//"cd" to change directory. No arguments sends to the home directory, while other arguments will search for the specified directory
	else if(!strcmp(args[0], "cd")){
		childWait();
		if(cnt == 1){
			chdir(getenv("HOME"));
		}
		else{
			if(chdir(args[1]) == -1){
				printf("Specified directory not found");
			}
		}
		return 0;
	}
//I decided to add this in myself, just so I could always have some sort of backup to determine which directory I found myself in.
	else if(!strcmp(args[0], "pwd")){
		childWait();
		if(cnt > 1){
			printf("Too many arguments!");
		}
		else{
			char cwd[1024];
			printf("%s\n", getcwd(cwd, sizeof(cwd)));
		}
		return 0;
	}
//"jobs" iterates through the list of backroung jobs and enumerates them all.
	else if(!strcmp(args[0], "jobs")){
		childWait();
		if(backProcess > 0){
			ProcessList *curr = List->next;
			while(curr != NULL){
				printf("\n%d. PID: %d; Command: %s", curr->number, curr->pid, curr->args);
				curr = curr->next;
			}
		}
		else{
			printf("No background jobs found!");
		}
		return 0;
	}
//"fg" takes the background job (specified by number, not pid), and removes it from the list. It then reformats the list so that the structure remains.
	else if(!strcmp(args[0], "fg")){
		childWait();
		if(cnt != 2){
			printf("Bad arguments, please try again.");
		}
		else if(backProcess == 0){
			printf("No background jobs to bring forward.");
		}
		else{
			int wanted = atoi(args[1]); //Turn the given number form a string to an int
			ProcessList *prev = List;
			ProcessList *curr = List->next;
			while(curr != NULL){
				if(curr->number == wanted){
					if(prev == NULL){
						List = curr->next;
						curr = NULL;
					}
					else{
						prev->next = curr->next;
						curr = NULL;
					}
					backProcess--;
					reformat();
					return 2;
				}
				prev = curr;
				curr = curr->next;
			}
		}
		return 0;
	}
	else{
		return 1;
	}
}

//The waiting function that was asked of us in the assignment ammendment
static void childWait(){
	int w, rem;
	w = rand() % 10;
	rem = sleep(w);
	while(rem != 0){
		rem = sleep(rem);
	}
}

//Signal handler, works on SIGINT and SIGTSTP
static void sigHandler(int sig){
	
//Kill current running thread when CTRL+C
	if(sig == SIGINT){
		int pthread_cancel(pthread_t thread);
	}
	
//Do nothing when CTRL+Z
	if(sig == SIGTSTP){
		//Do nothing
	}
	
}

int main(){

	if(signal(SIGINT, sigHandler) == SIG_ERR){
		printf("Error, could not bind signal handler");
	}

	if(signal(SIGTSTP, sigHandler) == SIG_ERR){
		printf("Error, could not bind signal handler");
	}

	char *args[64]; //where the command will be stored
	int bg; //boolean value for whether or not background is specified
	int fd; //File descriptor in case of redirection

	command = NULL;
	console = dup(STDOUT_FILENO);

//Initialize the list of background jobs
	List = malloc(sizeof(ProcessList));

//The eternal loop
	while(TRUE){
//Reset stdout so file redirections are not permanent
		dup2(console, STDOUT_FILENO);
		stdoutRedirect = 0;
		
		int cnt = getcmd("\nsh>>", args, &bg);
		
//Empty input resets the loop
		if(noinput == 1){
			continue;
		}

//Have a NULL pointer after args[] to ensure that reading stops
		args[cnt] = NULL;

//Handle output redirection
		if(stdoutRedirect == 1){
			fd = open(stdoutlocation, O_CREAT | O_RDWR | O_APPEND, 0600);
			dup2(fd, 1);
			close(fd);
		}

//Handle built-in functions, only if they're meant for the foreground
		int check = builtin(args, cnt);
		if(check == 0 || check == 2){
			continue;
		}

		pid_t pid = fork();

		if(pid == -1){
			perror("Fork Error: ");
		}
		else if(pid == 0){
			childWait();
			if(execvp(args[0], args) == -1){
				perror("Error: ");
				exit(EXIT_FAILURE);
			}
			else{
				exit(EXIT_SUCCESS);
			}
		}
		else{
			if(bg == 0){
				int status = 0;
				waitpid(pid, &status, 0);
			}
			else{
				insertbg(List, pid, command);
			}
		}
	}
}
