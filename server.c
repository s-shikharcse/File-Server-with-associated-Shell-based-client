#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm/errno.h>

#define MAXCON 1500

void error(char *msg) {
    perror(msg);
}

int childrenPID[MAXCON];

int file_buffer_size = 1024;
int fname_size = 21;

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[256], fname[fname_size], ch, file_buffer[file_buffer_size];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i;
	if (argc < 2) {
	    fprintf(stderr,"ERROR, no port provided\n");
	    exit(1);
	}

	/* create socket */

	sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);  //create a listening socket
	if (sockfd < 0) 
	   error("ERROR opening socket");

	/* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	/* bind socket to this port number on this machine */

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
		exit(1);
	}
	
	/* listen for incoming connection requests */

	listen(sockfd, MAXCON);		    // start listening. At most 200 requests queued up
	clilen = sizeof(cli_addr);

	for (i = 0; i < MAXCON; ++i)
	{
		childrenPID[i] = -1;
	}
	i=0;

	/* accept a new request, create a newsockfd */
	while(1) {
		//printf("Creating new process by %d\n", getpid());
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		
		if(newsockfd > 0)
		{		//connection request has come

			pid_t childPID = fork();

			if(childPID == 0) {
				/* read message from client */

				bzero(buffer,256);
				n = read(newsockfd, buffer, fname_size);
				if (n < 0) {
				   error("ERROR reading from socket");
				   exit(1);
				}

				for(i = 0 ; i < fname_size ; i++)
					fname[i] = buffer[i + 4];

				for(i = 0 ; i < fname_size ; i++)
					if(fname[i] == '\n')
						fname[i] = 0;

				FILE *fp;
				fp = fopen(fname, "r");
				if(fp == NULL) {
					char errormsg[500];
					sprintf(errormsg,"Error while opening the file, requested file: %s", fname);
					error(errormsg);
					exit(1);
				}
				// else
				// 	printf("File %s  opened successfully\n", fname);

				int curr = 0;
				bzero(file_buffer, file_buffer_size);

				while((ch = fgetc(fp)) != EOF) {
					if(curr == file_buffer_size) {
						n = write(newsockfd, file_buffer, file_buffer_size);
						bzero(file_buffer, file_buffer_size);
						if (n < 0) {
						   error("ERROR writing to socket");
						   exit(1);
						}
						curr = 0;
					}
					file_buffer[curr++] = ch;
				}
				if(curr != 0) {
					n = write(newsockfd, file_buffer, file_buffer_size);
					// printf("Sent bytes: %d\n", n);
					bzero(file_buffer, file_buffer_size);
					if (n < 0) {
					   error("ERROR writing to socket");
					   exit(1);
					}
				}
				fclose(fp);
				close(newsockfd);
///DEBUG				printf("Close status is :%d for process:%d \n",close(newsockfd), getpid());
///DEBUG				printf("File transfer over\n");
				exit(0);
			}
			else
			{
				close(newsockfd);
				int i;
				int found=0;
				for(i=0;i<MAXCON;++i)
				{
					if(childrenPID[i] == -1)
					{
						childrenPID[i] = childPID;
						found=1;
						break;
					}
				}
				if(!found)
					{
						printf("!!!!!!!! The pid array is full !!!!!!!!! \n The array is:\n");
						for(i=0;i<MAXCON;++i)
						{
							printf("%d ",childrenPID[i] );
						}
						printf("\n");
						exit(2);
					}
			}
		}
		//reap children here
		int status ;
		int i;
		for(i=0;i<MAXCON;++i)
		{
			if(childrenPID[i] != -1 && waitpid(childrenPID[i], &status , WNOHANG)>0)
			{
				childrenPID[i] = -1;
			}
		}


	}

	return 0; 
}
