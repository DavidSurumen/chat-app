/* Server side C/C++ program to demonstrate Socket
 * programming
 *
 * Author:  Our group
 * Last Modified:  Yesterday
 *
 */
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "helper.h"
#include "myqueue.h"

#define SA struct sockaddr
#define MAX 256
#define MAX_PENDING 5

int service = 1;

pthread_t pool[MAX_PENDING];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conditional_var = PTHREAD_COND_INITIALIZER;

int main(int argc, char const* argv[])
{
	int port;
	int server_fd, new_socket;
	struct sockaddr_in address;
	int addrlen;

	if (argc != 2){
		fprintf(stderr,"wrong arguments enter: program_name port\n");
		exit(-1);
	}

	port = atoi(argv[1]);

	/* build address data structure */
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	addrlen = sizeof(address);

	/* setup passive open */
	if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: socket error");
		exit(1);
	}
	if ((bind(server_fd, (SA*)&address, sizeof(address))) < 0) {
		perror("server: bind error");
		exit(1);
	}
	listen(server_fd, MAX_PENDING);

	if (listen(server_fd, MAX_PENDING) < 0) {
		perror("listen error");
		exit(1);
	}
	else
		printf("Waiting for customers\n");

	/* Create threadpool for handling multiple client requests */
	for(int j=0;j<MAX_PENDING;j++){
		pthread_create(&pool[j],NULL,thread_function,NULL);
	}

	/* main server loop */
	while(service) {

		if ((new_socket	= accept(server_fd,\
					(SA*)&address,\
					(socklen_t*)&addrlen)) < 0) {
			perror("server: accept");
			exit(1);
		}
		int *pclient = malloc(sizeof(int));
		*pclient=new_socket;

		pthread_mutex_lock(&mutex);
		enqueue(pclient);  /* making a queue of active client connections */
		pthread_cond_signal(&conditional_var);
		pthread_mutex_unlock(&mutex);
	}

	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
