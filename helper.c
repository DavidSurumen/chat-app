#include "helper.h"
#include "myqueue.h"
#include "operations.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t conditional_var;


// Consider an Inventory System.
/**
 * ==>  hold store number
 * ==>  item codes and item names
 * ==>  item quantities.
 *
 * ==>  Every user should be able to see this information, and update it
 *
 *
 */

/**
 *  One instance (one node in the linked list) ==> one record.
 *  Each node of the linked list represents a record. Store number, item code, item name, quantity
 *
 *  Each node in the linked list is an item in the Inventory.
 *
 *
 */


// linked list head
data *list_head = NULL;

/**
 * thread_function - description..
 * @arg:  ---
 * Return: void
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
	4. Search for Item\n";

	int logged_in = 0;

	/*infinite loop for chat*/
	for (;;) {
		memset(buff,0, sizeof(buff));
		recv(connfd, buff, sizeof(buff), 0);

		if (strncmp("login", buff, 5) == 0) {
			logged_in = 1;
			printf("Connected\n");
		}

		/*send something specific for this customer*/
		if (logged_in) {
			if (*buff == '1') {
				char *prompt = "Enter name of the item";
				char *status = "";
				// Add an item
				send(connfd, prompt, strlen(prompt) + 1, 0);
				recv(connfd, buff, sizeof(buff), 0);
				if (create_node(buff, &list_head) == 0) {
					status = "Success!";
					send(connfd, status, strlen(status) + 1, 0);
				} else {
					status = "Try again!";
					send(connfd, status, strlen(status) + 1, 0);
				}
			} else if (*buff == '2') {
				// Delete an item
				char *prompt = "Enter name of item to delete";
				char *msg;

				send(connfd, prompt, strlen(prompt) + 1, 0);
				recv(connfd, buff, sizeof(buff), 0);

				if (delete_item(buff, &list_head) == 0) {
					msg = "Deleted Successfully!";
					send(connfd, msg, strlen(msg) + 1, 0);
				} else {
					msg = "Item Not in Store.";
					send(connfd, msg, strlen(msg) + 1, 0);
				}
			} else if (*buff == '3') {
				// List all items
				char items[MAX];
				items[0] = '\0';  // initialize the buffer
				char *msg = "Store is Empty!";

				if(read_items(&list_head, items, sizeof(items)) == 0) {
					// reading was successful
					send(connfd, items, strlen(items) + 1, 0);
				} else
					send(connfd, msg, strlen(msg) + 1, 0);
			} else if (*buff == '4') {
				// search for a specific item
				char *prompt = "Enter Item to search";
				send(connfd, prompt, strlen(prompt) + 1, 0);
				recv(connfd, buff, sizeof(buff), 0);

				if (search(buff, list_head) == 0) {
					/*char *msg = strcat("Found! ", buff);*/
					/*char *msg = strcat(buff, " has been impounded!");*/
					char msg[100];
					memset(&msg, 0, sizeof(msg));
					strcat(msg, "Found! ");
					strcat(msg, buff);
					strcat(msg, " has been impounded.");
					send(connfd, msg, strlen(msg) + 1, 0);
				} else {
					char *msg = "Not Found.";
					send(connfd, msg, strlen(msg) + 1, 0);
				}

			} else if (strncmp("exit", buff, 4) == 0) {
				/*if msg contains "Exit" then server exit and chat ended.*/
				printf("Server Exit for customer\n");
				send(connfd, "exit", strlen("exit") + 1, 0);
				break;
			}
			send(connfd, menu, strlen(menu) + 1, 0);

		} else if (strncmp("exit", buff, 4) == 0) {
			/*if msg contains "Exit" then server exit and chat ended.*/
			printf("Server Exit for customer\n");
			send(connfd, "exit", strlen("exit") + 1, 0);
			break;
		} else {
			char *msg = "Please login.";
			send(connfd, msg, strlen(msg) + 1, 0);
		}
	}
	close(connfd);
	return 0;
}
