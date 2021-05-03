//////////////////////////////////////////////////////////////////////
//	filename: 	database.h
//
//	purpose:	Houses a library that provides functions to create,
//                      manage, and destroy temporary databases
//                      (stored locally in RAM space).
/////////////////////////////////////////////////////////////////////


#ifndef P3_DATABASE_H
#define P3_DATABASE_H

#include <stdbool.h>

typedef struct node_ {
	const char *key, *data;
	struct node_ *next;
} node_;

#include <pthread.h>
typedef struct linkedlist_ {
	int size;
	node_ *head, *rear;
	pthread_mutex_t lock;
} linkedlist_;

typedef struct hashtable_ {
	int size, capacity, num_working;
	volatile bool wr_ready;
	linkedlist_ **table;
} hashtable_;


hashtable_* hashtable_init();

void insertData(hashtable_ * hashtable, char *key, char *data);

char* getData(hashtable_ *hashtable, char *key);

int removeData(hashtable_ *hashtable, char * key);

void hashtable_destroy(hashtable_ *hasTable);


#endif //P3_DATABASE_H