/**
 * Routines for operations of the server, in a multithreaded application,
 * that uses a thread-safe in-memory linked list data structure and a shared
 * file storage to persist its data.
 *
 * Author: David Surumen and The Group (Collab)
 * Last Modified: 30th Nov 2023
 *
 */
#include "operations.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

extern pthread_mutex_t mutex;

/**
 * create_node - adds a new entry into the linked list shared data structure
 * @name: string that represents the item name
 * @head: double pointer to the head of the linked list
 *
 * Return: 0 if success -1
 */
int create_node(char *name, data **head)
{
	int i = 1;

	if (strcmp(name, "") ==0)
		return -1;  // client only entered an empty string for item name
	if (!*head)
		build_from_file("linked_list_data.txt", *head);

	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr, "Memory allocation failed\n");
		return -1;
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
	return 0;  // success
}

/**
 * search - searches for an item in the linked list
 * @name: string -the name of the item that is to be searched for
 * @head: struct data -double pointer to the head of the linked list
 *
 * Return: 0 (Success), -1 (Failure)
 *
 */
int search(char *name, data *head)
{
	if (!head)
		build_from_file("linked_list_data.txt", head);

	data *tmp = head;
	printf("Searching for %s...\n", name);
	if (!head)
		return -1;  // list is empty

	while(tmp) {
		if (strcmp(tmp->item_name, name) == 0)
			return 0;  // found the item
		tmp = tmp->next;
	}
	return -1; // item not found
}

/**
 * read_items - reads all the items in the linked list
 * @head: struct data -double pointer to head of the linked list
 * @items_buffer: string -pointer to an array of characters of the calling function, in which
 * to store the read item names of each node
 * @max_length: unsigned int -size of the items buffer
 *
 * Return: 0 (Success), -1 (Failure)
 */
int read_items(data **head, char *items_buffer, size_t max_length)
{
	if (!*head)
		build_from_file("linked_list_data.txt", *head);

	data *tmp = *head;
	size_t total_length = 0;

	if (!tmp)
		return -1;

	printf("Listing Items...\n");
	strcat(items_buffer, "\nITEMS IN STORE\n");
	strcat(items_buffer, "---------------\n");
	while (tmp) {
		size_t item_length = strlen(tmp->item_name);

		// check if there is enough space in buffer
		if (total_length + item_length + 1 <= max_length) {
			// concatenate current item_name and '\n' to the buffer
			strcat(items_buffer, tmp->item_name);
			strcat(items_buffer, "\n");
			total_length += item_length + 1;
		} else
			break;  // case where the buffer is filled up
		tmp = tmp->next;
	}
	return 0;
}

/**
 * reload_items - recreates the in-memory linked list
 * @head: struct data -double pointer to the head of the linked list
 * @item: string -item name for the node that will be re-created.
 * @rank: int -an indicator for whether or not the item that is passed is the first or not
 *
 * Return: 0 (Success), -1 (Failure)
 */
int reload_items(data **head, char *item, int rank)
{
	data *newnode = (data*)malloc(sizeof(data));
	data *current;

	if (!newnode) {
		fprintf(stderr,"memory allocation failed\n");
		return -1;
	}
	newnode->item_name = strdup(item);
	newnode->next = NULL;
	pthread_mutex_init(&newnode->mutex, NULL);

	pthread_mutex_lock(&mutex);  // lock the linked list to add nodes
	if (rank == 1) {
		// rank 1 means the item is the first in the file, therefore it makes the head.
		newnode->next = *head;
		*head = newnode;
	} else {
		current = *head;
		while (current->next){
			current = current->next;
		}
		current->next = newnode;
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

/**
 *
 *
 */
int delete_item(char *name, data **head)
{
	data *tmp = *head;
	data *fixer;
	int found = 0;

	if (!tmp)
		return -1;

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
			printf("Deleting %s...\n", name);

			fixer = tmp->next;
			tmp->next = tmp->next->next;

			free(fixer->item_name);
			free(fixer);
			save(*head);
			break;  // successfully deleted
		}
		tmp = tmp->next;
	}
	if (found != 1)
		return -1; // failed to delete
	return 0;
}

/**
 *
 *
 */
int save(data *head)
{
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

/**
 *
 *
 */
int lock_file(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	return fcntl(fd, F_SETLK, &lock);
}

/**
 *
 *
 */
int unlock_file(int fd)
{
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	return fcntl(fd, F_SETLK, &lock);
}

/**
 *
 *
 */
int build_from_file(const char *filename, data *head)
{
	int count = 1;
	FILE *file = fopen(filename, "r");
	if (file == NULL)
		return -1;

	char contents[100];

	while (fgets(contents, sizeof(contents), file) != NULL) {
		size_t len = strlen(contents);
		if (len > 0 && contents[len - 1] == '\n') {
			contents[len - 1] = '\0';
		}
		if (contents[0] && contents[0] != '\n') {
			if (reload_items(&head, contents, count) != 0)
				return -1;
			count++;
		}
	}
	return 0;
	fclose(file);
}
