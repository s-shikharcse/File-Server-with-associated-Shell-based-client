#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_COMMANDS 11
#define MAX_PROCESSES 64

static char presentDir[MAX_TOKEN_SIZE];
static char commands[MAX_COMMANDS][MAX_TOKEN_SIZE];
static char server[MAX_TOKEN_SIZE], serverport[MAX_TOKEN_SIZE];
static char line[MAX_INPUT_SIZE];
static int bg_process[MAX_PROCESSES], fg_process[MAX_PROCESSES];

static char **tokens;

void freemem() {
	int i;
	for(i = 0 ; tokens[i] != NULL ; i++)
		free(tokens[i]);
	free(tokens);
}

void reapChild(int childPID) {
	int i, status;
	for(i = 0 ; i < MAX_PROCESSES ; i++) {
		if(fg_process[i] == childPID) {
			waitpid(childPID, &status, 0);
			fg_process[i] = -1;
		}
	}
}

void assignPID(int childPID) {
	int i;
	for(i = 0 ; i < MAX_PROCESSES ; i++) {
		if(fg_process[i] == -1) {
			fg_process[i] = childPID;
			return;
		}
	}
}

char **tokenize(char *line, int* tokensfound) {
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for(i =0; i < strlen(line); i++){

	char readChar = line[i];

	if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
		token[tokenIndex] = '\0';
		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
		strcpy(tokens[tokenNo++], token);
		tokenIndex = 0; 
		}
	}
	else
		token[tokenIndex++] = readChar;
	}
 
	free(token);
	tokens[tokenNo] = NULL ;
	*tokensfound = tokenNo;
	return tokens;
}

int is_standard(char* token) {
	DIR *dir;
	struct dirent *ent;
	int found = 0;
	if ((dir = opendir("/bin/")) != NULL) {					
		while ((ent = readdir(dir)) != NULL) {
			if(!strcmp(token, ent->d_name)) {
				found =1;
				break;
			}
		}
		closedir(dir);
	}
	else {
		fprintf(stderr, "Could not open /bin/\n");
		return 0;
	}
	////////couldn't find in /bin, so now check in /usr/bin/
	DIR *dir2;
	struct dirent *ent2;
	if ((dir2 = opendir("/usr/bin/")) != NULL) {
		while ((ent2 = readdir(dir2)) != NULL) {
			if(!strcmp(token , ent2->d_name)){
				found = 2; 
				break;
			}
		}
		closedir(dir2);
	}
	else {
		fprintf(stderr, "Could not open /usr/bin/\n");
		return 0;
	}
	return found;
}

int isGetflWithPipe(char ** tokens, int tokensfound) {
	if(tokensfound <= 2)
		return 0;
	if(strcmp(tokens[0], commands[2]))
		return 0;

	char curr_token[MAX_TOKEN_SIZE];
	sprintf(curr_token,"|");
	if(strcmp(tokens[2], curr_token))
		return 0;
	return 1;
}

int isGetflWithRedirection(char ** tokens, int tokensfound) {
	if(tokensfound <= 2)
		return 0;
	if(strcmp(tokens[0], commands[2]))
		return 0;

	char curr_token[MAX_TOKEN_SIZE];
	sprintf(curr_token,">");
	if(strcmp(tokens[2], curr_token))
		return 0;
	return 1;
}

void run_command(char ** tokens, int tokensfound) {
	//if empty command, do nothing
	if(tokens[0] == NULL)
		return;

	//Case of exit command
	if(!strcmp(tokens[0], commands[10])) {
		if(tokensfound > 1) {
			fprintf(stderr,"Wrong use of exit. Usage: exit\n");
			return ;
		}
		int i;
		for(i = 0 ; i < MAX_PROCESSES ; i++) {
			if(bg_process[i] != -1) {
				kill(bg_process[i], SIGINT);
			}
			bg_process[i] = -1;
		}
		freemem();
		exit(0);
	}

	//Case of cd command
	if(!strcmp(tokens[0], commands[0])) {
		//if more tokens are present, then it is an error
		if(tokensfound != 2) {
			fprintf(stderr,"Wrong use of %s. Usage: cd directoryname\n", tokens[0]);
			return ;
		}
		//try to change the directory
		if(chdir(tokens[1])) {
			fprintf (stderr, "%s: %s\n",tokens[0], strerror (errno));
			return;
		}
		
		getcwd(presentDir, MAX_TOKEN_SIZE);
		printf("Directory changed to: %s\n", presentDir);
		return;
	}

	//Case of server command
	if(!strcmp(tokens[0], commands[1])) {
		if(tokensfound != 3) {
			fprintf(stderr,"Wrong use of %s. Usage: server <server-IP> <server-port>\n", tokens[0]);
			return;
		}
		bzero(server, MAX_TOKEN_SIZE);
		bzero(serverport, MAX_TOKEN_SIZE);
		strcpy(server, tokens[1]);
		strcpy(serverport, tokens[2]);
		printf("Set server: %s, Server-port: %s\n",server, serverport);
		return;
	}
	//We run only the cd command and the server command in the main shell. All other commands are executed in a child shell

	//Case of getsq command
	if(!strcmp(tokens[0], commands[3])) {
		if(tokensfound <= 1) {
			fprintf(stderr,"Please enter at least 1 filename");
			return;
		}
		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}

		int i, childPID, status;
		for(i = 1 ; i < tokensfound ; i++) {
			if((childPID = fork()) == 0) {
				setpgid(0, 0);
				execl("./get-one-file-sig", "./get-one-file-sig", tokens[i], server, serverport, "nodisplay", (char*) NULL);
				exit(1);
			}
			else {
				assignPID(childPID);
				reapChild(childPID);
				// waitpid(childPID, &status, 0);
			}
		}
		return;
	}

	//Case of getbg command
	if(!strcmp(tokens[0], commands[9])) {
		if(tokensfound != 2) {
			fprintf(stderr,"Wrong use of %s. Usage: %s <filename>\n", tokens[0], tokens[0]);
			return;
		}

		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}
		int childPID;
		if((childPID = fork()) == 0) {
			setpgid(0, 0);
			execl("./get-one-file-sig", "./get-one-file-sig", tokens[1], server, serverport, "nodisplay", (char*) NULL);
			exit(1);	// in case execl fails
		}
		else {
			int i;
			for(i = 0 ; i < MAX_PROCESSES ; i++) {
				if(bg_process[i] == -1) {
					bg_process[i] = childPID;
					break;
				}
			}
			return;
		}
	}

	//Case of getpl command
	if(!strcmp(tokens[0], commands[4])) {
		int children[tokensfound - 1], cnt = 0;
		if(tokensfound <= 1) {
			fprintf(stderr,"Please enter at least 1 filename");
			return;
		}
		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}

		int i, childPID, status;
		for(i = 1 ; i < tokensfound ; i++) {
			if((childPID = fork()) == 0) {
				setpgid(0, 0);
				execl("./get-one-file-sig", "./get-one-file-sig", tokens[i], server, serverport, "nodisplay", (char*) NULL);
				exit(1);
			}
			else {
				assignPID(childPID);
				children[cnt++] = childPID;
			}
		}
		for(i = 0 ; i < tokensfound - 1 ; i++)
			reapChild(children[i]);
			// waitpid(children[i], &status, 0);
		return;
	}

	//Case of getfl | command
	if(isGetflWithPipe(tokens, tokensfound)) {
		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}

		int p[2];
		if(pipe(p) < 0) {
			printf("Pipe error\n");
			return;
		}

		int child1, child2, status;
		if((child1 = fork()) == 0) {
			setpgid(0, 0);
			close(1);
			dup(p[1]);
			close(p[0]);
			close(p[1]);
			execl("./get-one-file-sig", "./get-one-file-sig", tokens[1], server, serverport, "display", (char*) NULL);
			exit(1);
		}
		if((child2 = fork()) == 0) {
			setpgid(0, 0);
			close(0);
			dup(p[0]);
			close(p[0]);
			close(p[1]);
			char command[MAX_INPUT_SIZE];
			bzero(command, MAX_INPUT_SIZE);
			sprintf(command, "/bin/");
			sprintf(command + 5, tokens[3]);
			execv(command, tokens + 3);
			exit(1);
		}
		assignPID(child1);
		assignPID(child2);
		close(p[0]);
		close(p[1]);
		reapChild(child2);
		reapChild(child1);
		return;
	}

	//Case of getfl > outputfile command
	if(isGetflWithRedirection(tokens, tokensfound)) {
		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}

		int filefd;
		
		int childPID, status;
		if((childPID = fork()) == 0) {
			setpgid(0, 0);
			close(1);
			filefd = open(tokens[3], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXO | S_IRWXU | S_IRWXG);
			execl("./get-one-file-sig", "./get-one-file-sig", tokens[1], server, serverport, "display", (char*) NULL);
			exit(1);
		}
		assignPID(childPID);
		reapChild(childPID);
		return;
	}

	//Case of getfl command
	if(!strcmp(tokens[0], commands[2])) {
		if(tokensfound != 2) {
			fprintf(stderr,"Wrong use of %s. Usage: %s <filename>\n", tokens[0], tokens[0]);
			return;
		}

		if(server[0] == 0 || serverport[0] == 0) {
			fprintf(stderr,"Server not configured. User server command to configure server\n");
			return;
		}
		int childPID, status;
		if((childPID = fork()) == 0) {
			setpgid(0, 0);
			execl("./get-one-file-sig", "./get-one-file-sig", tokens[1], server, serverport, "display", (char*) NULL);
			exit(1);	// in case execl fails
		}
		else {
			assignPID(childPID);
			reapChild(childPID);
			// waitpid(childPID, &status, 0);
			return;
		}
	}

	//Case of other commands
	if(is_standard(tokens[0])) {
		int childPID, status;
		if((childPID = fork()) == 0) {
			execvp(tokens[0], tokens);
			printf("Wrong Command\n");
		}
		else {
			assignPID(childPID);
			reapChild(childPID);
			return;
		}
	}

	//Command doesn't exist!
	printf("Command not found\n");
	return;
}

void setup() {
	getcwd(presentDir, MAX_TOKEN_SIZE);
	sprintf(commands[0], "cd");
	sprintf(commands[1], "server");
	sprintf(commands[2], "getfl");
	sprintf(commands[3], "getsq");
	sprintf(commands[4], "getpl");
	sprintf(commands[5], "ls");
	sprintf(commands[6], "cat");
	sprintf(commands[7], "echo");
	sprintf(commands[8], "exit");
	sprintf(commands[9], "getbg");
	sprintf(commands[10], "exit");
	int i;
	for(i = 0 ; i < MAX_PROCESSES; i++) {
		fg_process[i] = -1;
		bg_process[i] = -1;
	}
}

void sigproc() {
	signal(SIGINT, sigproc);
	int i;
	for(i = 0 ; i < MAX_PROCESSES ; i++) {
		if(fg_process[i] != -1) {
			kill(fg_process[i], SIGINT);
			fg_process[i] = -1;
		}
	}
}

void main(void) {
	signal(SIGINT, sigproc);
	setup();	
	int i, tokensfound, status;
	while (1) {		
		printf("Hello> ");
		bzero(line, MAX_INPUT_SIZE);
		fgets(line, MAX_INPUT_SIZE, stdin);
		line[strlen(line)] = '\n'; 					//terminate with new line
		tokens = tokenize(line, &tokensfound);
		 
		//do whatever you want with the commands, here we just print them

		run_command(tokens, tokensfound);

		for(i = 0 ; i < MAX_PROCESSES ; i++) {
			if(bg_process[i] != -1)
				if(waitpid(bg_process[i], &status, WNOHANG) == bg_process[i])
					bg_process[i] = -1;
		}
	
		// Freeing the allocated memory	
		freemem();
	}
}
