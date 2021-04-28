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
	char *key, *data;
	struct node_ *next;
} node_;

#include <pthread.h>
typedef struct linkedList_ {
	int size;
	node_ *head, *rear;
	pthread_mutex_t lock;
} linkedList_;

typedef struct hashTable_ {
	int size, capacity, num_working;
	volatile bool wr_ready;
	linkedList_ **table;
} hashTable_;


hashTable_* hashTable_init();

void insertData(hashTable_ * hashTable, char *key, char *data);

char* getData(hashTable_ *hashTable, char *key);

int removeData(hashTable_ *hashTable, char * key);

void hashTable_destroy(hashTable_ *hasTable);


#endif //P3_DATABASE_H