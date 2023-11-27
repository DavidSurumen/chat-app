#include "helper.h"
#include "myqueue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t conditional_var;

/**
 * - DOC -
 */
void *thread_function(void *arg) {
	while(1){
		int *pclient;
		pthread_mutex_lock(&mutex);
		if((pclient=dequeue())==NULL){
			pthread_cond_wait(&conditional_var,&mutex);
			/*try again*/
			pclient=dequeue();
		}
		pthread_mutex_unlock(&mutex);
		if(pclient!=NULL){
			chat(pclient);
		}
	}
}

/**
 * - DOC -
 */
void *chat(void *aconnfd)
{
	int connfd = *(int*)aconnfd;
	char buff[MAX];
	int n;

	char* menu = "Select Action\n\
	1. Add Item\n\
	2. Delete Item\n\
	3. List Items\n\
	4. Search for Item";

	/*infinite loop for chat*/
	for (;;) {

		memset(buff,0, sizeof(buff));
		printf("waiting for response from customer \n");
		recv(connfd, buff, sizeof(buff), 0);
		printf("From client : %s\t To client : %s\n",buff,buff);

		/*send something specific for this customer*/
		send(connfd, menu, strlen(menu) + 1, 0);
		/* send menu */
		printf("%s\n", menu);

		/*if msg contains "Exit" then server exit and chat ended.*/
		if (strncmp("exit", buff, 4) == 0) {
			printf("Server Exit for customer\n");
			break;
		}
	}
	close(connfd);
}
