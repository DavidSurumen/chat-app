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

char *read_items(data**);
void add_item(data**, char*, int);
int delete_item(char*, data**);
void persist_to_file(data*);
int lock_file(int);
int unlock_file(int);
int build_from_file(const char*);
void create_node(char *, data**);

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

	/*infinite loop for chat*/
	for (;;) {

		/*memset(buff,0, sizeof(buff));*/
		/*data *head = NULL;*/
		recv(connfd, buff, sizeof(buff), 0);

		if (*buff == '1') {
			char *prompt = "Enter name of the item";
			// implement logic to add item
			send(connfd, prompt, strlen(prompt) + 1, 0);
			
			recv(connfd, buff, sizeof(buff), 0);
			create_node(buff, &list_head);
		} else if (*buff == '2') {
			char *prompt = "Enter name of item to delete";
			char *msg;

			send(connfd, prompt, strlen(prompt) + 1, 0);
			recv(connfd, buff, sizeof(buff), 0);
			
			if (delete_item(buff, &list_head) == 0) {
				msg = "Deleted Successfully!";
				send(connfd, msg, strlen(msg) + 1, 0);
			} else {
				msg = "Failed! No item with that name";
				send(connfd, msg, strlen(msg) + 1, 0);
			}
		} else if (*buff == '3') {
			char *items = "";

			items = read_items(&list_head);
			if (!items)
				items = "There is no items yet.";
			send(connfd, items, strlen(items) + 1, 0);
		} else if (*buff == '4') {
			// search for a specific item
			char *prompt = "Enter Item to search";

			printf("\n\nThe client would like to search for an item\n\n");
			send(connfd, prompt, strlen(prompt) + 1, 0);
			recv(connfd, buff, sizeof(buff), 0);

		}

		/*printf("From client : %s\t To client : %s\n",buff,buff);*/

		/*send something specific for this customer*/
		send(connfd, menu, strlen(menu) + 1, 0);

		/*if msg contains "Exit" then server exit and chat ended.*/
		if (strncmp("exit", buff, 4) == 0) {
			printf("Server Exit for customer\n");
			break;
		}
	}
	close(connfd);
	return 0;
}

/**
 *
 *  -- DOC --
 *
 */
void create_node(char *name, data **head) {
	int i = 1;

	if (!*head)
		build_from_file("linked_list_data.txt");
	
	data *tmp = *head;
	printf("stored items\n");
	printf("---------------\n");
	while (tmp != NULL) {
		printf("%d: %s\n", i, tmp->item_name);
		/*if (tmp->next == NULL)*/
			/*break;*/
		tmp = tmp->next;
		i++;
	}
	printf("\n");

	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr, "Memory allocation failed\n");
		exit(1);
	}

	newnode->item_name = strdup(name);
	newnode->next = NULL;

	pthread_mutex_init(&newnode->mutex, NULL);

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

	persist_to_file(*head);
	printf("Added Item: %d: %s\n", i, newnode->item_name);
}

void add_item(data **head, char *item, int rank) {
	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr, "memory alloc failed\n");
		exit(1);
	}
	newnode->item_name = strdup(item);
	newnode->next = NULL;

	if (rank == 1) {
		newnode->next = *head;
		*head = newnode;
	} else {
		current = *head;
		while (current->next) {
			current = current->next;
		}
		current->next = newnode;
	}
}

/**
 *
 * -- DOC --
 *
 */
int delete_item(char *name, data **head) {
	data *tmp = head;
	data *fixer;
	int found = 0;

	while (tmp->next) {
		if (tmp->next->item_name == name) {
			fixer = tmp->next;
			tmp->next = tmp->next->next;
			free(fixer->item_name);
			free(fixer);
			found = 1;
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
char *read_items(data **head) {
	data *tmp = head;
	char *items_array[1024]; // declare an array to hold the item names
	int i = 0;
	int total_length = 1;
	int count = 0;

	while (tmp) {
		items_array[count] = tmp->item_name;
		tmp = tmp->next;
		count++;
	}

	if (!items_array[0]) {
		return NULL;
	}

	for (int i = 0; i < count; i++) {
		printf("%s\n", items_array[i]);
		total_length += strlen(items_array[i]) + 1;
	}

	char items[total_length];
	items[0] = '\0';

	for (int i = 0; i < count; i++) {
		strcat(items, items_array[i]);
		strcat(items, "\n");
	}
	return items;
}

/**
 *
 * -- DOC --
 *
 */
void persist_to_file(data *head) {
	int fd = open("linked_list_data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd == -1) {
		perror("Error opening file");
		exit(1);
	}

	// lock the file for writing
	if (lock_file(fd) == -1) {
		perror("Error locking file");
		close(fd);
		exit(1);
	}

	// write the linked list data to the file
	data *current = head;
	while (current != NULL) {
		// lock the node's mutex before accessing its data
		pthread_mutex_lock(&current->mutex);
		dprintf(fd, "%s\n", current->item_name);
		pthread_mutex_unlock(&current->mutex);

		current = current->next;
	}
	// unlock the file
	if (unlock_file(fd) == -1) {
		perror("Error unlocking file");
	}
	close(fd);
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
		puts("Creating Shared file...");
		return -1;
	}
	char contents[100];
	while (fgets(contents, sizeof(contents), file) != NULL) {
		size_t len = strlen(contents);
		if (len > 0 && contents[len-1] == '\n') {
			contents[len-1] = '\0';
		}

		if (contents[0] && contents[0] != '\n') {
			add_item(&list_head, contents, count);
			count++;
		}
	}
	return 0;
	fclose(file);
}
