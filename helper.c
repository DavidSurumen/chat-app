#include "helper.h"
#include "myqueue.h"
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

typedef struct data_storage {
	char *item_name;
	struct data_storage *next;
	pthread_mutex_t mutex;
} data;

int read_items(data**, char*, size_t);
int reload_items(data**, char*, int);
int delete_item(char*, data**);
int save(data*);
int lock_file(int);
int unlock_file(int);
int build_from_file(const char*);
int create_node(char *, data**);
int search(char*);

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

				if (search(buff) == 0) {
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

/**
 * Return: 0 (Item found) Otherwise -1
 */
int search(char *name) {
	if (!list_head)
		build_from_file("linked_list_data.txt");

	data *tmp = list_head;

	printf("Searching for %s...\n", name);
	if (!list_head) {
		return -1;  // list is empty
	}

	while(tmp) {
		if (strcmp(tmp->item_name, name) == 0)
			return 0;  // found the item
		tmp = tmp->next;
	}
	return -1;  // item not found
}

/**
 *
 *  -- DOC --
 *
 */
int create_node(char *name, data **head) {
	int i = 1;

	if (strcmp(name, "") == 0)
		return -1;  // client just entered an empty string for item name

	if (!*head)
		build_from_file("linked_list_data.txt");

	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}

	newnode->item_name = strdup(name);
	newnode->next = NULL;
	pthread_mutex_init(&newnode->mutex, NULL);

	pthread_mutex_lock(&mutex); // lock the linked list for addition of new node
	if (*head == NULL) {
		newnode->next = *head;
		*head = newnode;
	} else {
		current = *head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = newnode;
	}
	if (save(*head) != 0)
		return -1;
	pthread_mutex_unlock(&mutex);

	printf("Adding %s...\n", newnode->item_name);
	return 0; // success
}

int reload_items(data **head, char *item, int rank) {
	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr, "memory alloc failed\n");
		return -1;
	}
	newnode->item_name = strdup(item);
	newnode->next = NULL;
	pthread_mutex_init(&newnode->mutex, NULL);

	pthread_mutex_lock(&mutex);  // lock the linked list to add nodes
	if (rank == 1) {
		// rank 1 means the item is the first in the file, therefore makes the head
		newnode->next = *head;
		*head = newnode;
	} else {
		current = *head;
		while (current->next) {
			current = current->next;
		}
		current->next = newnode;
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

/**
 *
 * -- DOC --
 *
 */
int delete_item(char *name, data **head) {
	data *tmp = *head;
	data *fixer;
	int found = 0;

	if (!tmp)
		return 1;

	pthread_mutex_lock(&mutex);
	if (strcmp((*head)->item_name, name) == 0) {
		printf("Deleting %s...\n", name);
		*head = (*head)->next;
		free(tmp);
		save(*head);
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	pthread_mutex_unlock(&mutex);

	while (tmp->next) {
		if (strcmp(tmp->next->item_name, name) == 0) {
			found = 1;
			printf("Deleting %s\n", name);

			fixer = tmp->next;
			tmp->next = tmp->next->next;

			free(fixer->item_name);
			free(fixer);
			save(*head);
			break; // successfuly deleted
		}
		tmp = tmp->next;
	}
	if (found != 1)
		return 1; // failed to delete
       return 0;
}

/**
 *
 * -- DOC --
 *
 */
int read_items(data **head, char *items_buffer, size_t max_length) {
	if (!*head)
		build_from_file("linked_list_data.txt");

	data *tmp = *head;
	size_t total_length = 0;

	if (!tmp)
		return -1;

	printf("Listing Items...\n");
	strcat(items_buffer, "\nITEMS IN STORE\n");
	strcat(items_buffer, "---------------\n");
	while (tmp) {
		size_t item_length = strlen(tmp->item_name);

		// check if there is enough space in the buffer
		if (total_length + item_length + 1 <= max_length) {
			// concatenate current item_name and '\n' to the buffer
			strcat(items_buffer, tmp->item_name);
			strcat(items_buffer, "\n");
			total_length += item_length + 1;
		} else {
			// case where the buffer is filled up or not large enough
			break;
		}
		tmp = tmp->next;
	}
	return 0;
}

/**
 *
 * -- DOC --
 *
 */
int save(data *head) {
	int fd = open("linked_list_data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("Error opening file");
		return -1;
	}

	// lock the file for writing
	if (lock_file(fd) == -1) {
		perror("Error locking file");
		close(fd);
		return -1;
	}

	// write the linked list data to the file
	data *current = head;
	while (current != NULL) {
		// lock the node's mutex before accessing its data
		dprintf(fd, "%s\n", current->item_name);

		current = current->next;
	}
	// unlock the file
	if (unlock_file(fd) == -1) {
		perror("Error unlocking file");
	}
	close(fd);
	return 0;
}

int lock_file(int fd) {
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	return fcntl(fd, F_SETLK, &lock);
}

int unlock_file(int fd) {
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	return fcntl(fd, F_SETLK, &lock);
}

int build_from_file(const char *filename) {
	int count = 1;
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return -1;
	}
	char contents[100];

	while (fgets(contents, sizeof(contents), file) != NULL) {
		size_t len = strlen(contents);
		if (len > 0 && contents[len-1] == '\n') {
			contents[len-1] = '\0';
		}

		if (contents[0] && contents[0] != '\n') {
			if (reload_items(&list_head, contents, count) != 0)
				return -1;
			count++;
		}
	}
	return 0;
	fclose(file);
}
