#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>

void error(char *msg)
{
	perror(msg);
	exit(0);
}

int bytes_read = 0;

void sigproc()
{ 		 signal(SIGINT, sigproc); /*  */
		 /* NOTE some versions of UNIX will reset signal to default
		 after each call. So for portability reset signal each time */
 
		 printf("Received SIGINT; downloaded %d bytes so far.\n", bytes_read);
		 exit(1);
}

int file_buffer_size = 1024;

int main(int argc, char *argv[])
{
	signal(SIGINT, sigproc);
	int i;

	// printf("Arguments\n");
	// printf("argc = %d\n", argc);
	// for(i = 0 ; i < argc ; i++)
	// 	printf("%s\n", argv[i]);

	int sockfd, portno, n;

	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256], file_buffer[file_buffer_size];

	if (argc < 5) {
	   fprintf(stderr,"usage!!!! %s filename hostname port display_mode[nodisplay/display]\n", argv[0]);
	   exit(0);
	}

	/* create socket, get sockfd handle */

	portno = atoi(argv[3]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	/* fill in server address in sockaddr_in datastructure */

	server = gethostbyname(argv[2]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_port = htons(portno);

	/* connect to server */

	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR connecting");

	/* ask user for input */

	bzero(buffer,256);
	// fgets(buffer,255,stdin);
	buffer[0] = 'g';
	buffer[1] = 'e';
	buffer[2] = 't';
	buffer[3] = ' ';
	sprintf(buffer + 4, argv[1]);

	int tmp;
	for(tmp = 0 ; tmp < 256 ; tmp++)
		if(buffer[tmp] == '\n')
			buffer[tmp] = 0;
		
	/* send user message to server */

	n = write(sockfd,buffer,strlen(buffer));
	if (n < 0) 
		 error("ERROR writing to socket");
	bzero(buffer,256);

	/* read reply from server */

	while((n = read(sockfd, file_buffer, file_buffer_size - 1)) > 0) {
		if(argv[4][0] == 'd')
			printf("%s", file_buffer);
		bytes_read += n;
		bzero(file_buffer, file_buffer_size);
	}
	// printf("Successfully completed\n");
	exit(0);
}
