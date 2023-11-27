/* Client side TCP/IP C program to demonstrate Socket
 * programming
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SA struct sockaddr
#define MAX 256

void chat(int);// for interacting with server

/**
 *  - DOC -
 */
int main(int argc, char *argv[])
{
	char * hostname;
	int port,i;
	if (argc != 3){
		/*fprintf(stderr,"wrong arguments enter: program_name hostname port\n");*/
		fprintf(stderr, "Usage: ./<program_name> <hostname> <port>\n");
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]);

	/* translate host name into peer’s IP address */
	struct in_addr a;
	struct hostent *serv = gethostbyname(hostname);
	/* TODO: Use getaddrinfo() instead. gethostbyname() is obsolete */

	if (!serv) {
		fprintf(stderr, "client: unknown host: %s\n", hostname);
		exit(1);
	}
	/* print translated server IP address not necessary*/
	for (i = 0; serv->h_addr_list[i] != NULL; i++) {
		a.s_addr = *((uint32_t*) serv->h_addr_list[i]);
		printf("IP[%d]: %s\n\n", i, inet_ntoa(a));
	}

	/* build address data structure */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	memcpy(&addr.sin_addr.s_addr, serv->h_addr, serv->h_length);
	/* manual entry addr.sin_addr.s_addr = inet_addr("127.0.0.1"); */

	/* active open */
	int sockfd;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("client: socket error");
		exit(1);
	}
	/* connect the client socket to server socket */
	if (connect(sockfd, (SA*)&addr, sizeof(addr)) < 0) {
		perror("client: connect error");
		close(sockfd);
		exit(1);
	}
	/* function for chat */
	chat(sockfd);

	close(sockfd);
	return 0;
}

/**
 *  - DOC -
 */
void chat(int sockfd)
{
	char buff[MAX];
	int n;

	for (;;) {
		/* send something to server such as login credentials */
		memset(buff,0, sizeof(buff));
		printf("Enter the string : ");

		n = 0;
		while ((buff[n++] = getchar()) != '\n');

		buff[n-1] = '\0';
		send(sockfd, buff, strlen(buff) + 1, 0);

		/* read response from server */
		memset(buff,0, sizeof(buff));
		printf("waiting for response.........\n");

		recv(sockfd, buff, sizeof(buff), 0);
		printf("From server: %s\n",buff);

		if ((strncmp(buff, "exit", 4)) == 0) {
			printf("Client Exit...\n");
			break;
		}
	}
}
