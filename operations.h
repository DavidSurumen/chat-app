#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <pthread.h>

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
int build_from_file(const char*, data*);
int create_node(char*, data**);
int search(char*, data*);

#endif /* OPERATIONS_H */
