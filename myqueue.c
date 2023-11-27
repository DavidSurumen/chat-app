#include "myqueue.h"
#include <stdlib.h>

node_t* head =NULL;  /* queue head */
node_t* tail =NULL;  /* queue tail */

/**
 * enqueue - adds client sockets with active connections  to the queue
 *
 * client_socket: file descriptor for an active client socket connection
 * Return: void (NULL)
 */
void enqueue(int *client_socket){
	node_t *newnode = malloc(sizeof(node_t));
	newnode->client_socket = client_socket;
	newnode->next = NULL;

	if (tail == NULL){
		head = newnode;
	} else {
		tail->next = newnode;
	}
	tail = newnode;
}

/**
 * - DOC -
 */
int* dequeue(){
	if(head==NULL){
		return NULL;
	} else {
		int *result= head->client_socket;
		node_t *temp=head;
		head=head->next;
		if(head==NULL){tail=NULL;}
		free(temp);
		return result;
	}
}
