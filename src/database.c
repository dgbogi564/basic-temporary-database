#include "database.h"

#include <string.h>

#include "macros.h"

#ifndef HASH
#define HASH 10
#endif

// TODO Make sure to check if any function returns NULL or 1 and send the appropriate error code to the client and sever the connection if needed.


/* ============================ LINKEDLIST & NODES =============================== */


node_* node_init(char *key, char *data) {
	node_ *node = safe_malloc(__func__, sizeof(node_));
	node->key = key;
	node->data = data;
	return node;
}

void node_insert(linkedList_ *linkedList, char *key, char *data) {
	pthread_mutex_lock(&linkedList->lock);

	if (linkedList->head == NULL)
		linkedList->head = linkedList->rear = node_init(key, data);
	else {
		node_ *node = linkedList->head, *prev = NULL;
		int retval;
		while (node != NULL && (retval = strcmp(key, node->key)) > 0) {
			prev = node;
			node = node->next;
		}

		if (prev == NULL) {
			node_ *temp = linkedList->head;
			linkedList->head = node_init(key, data);
			linkedList->head->next = temp;
		} else {
			if (node != NULL) {
				if (retval) {
					prev->next = node_init(key, data);
					prev->next->next = node;
				} else
					node->data = data;
			} else
				linkedList->rear = linkedList->rear->next = node_init(key, data);
		}
	}

	pthread_mutex_unlock(&linkedList->lock);
}

linkedList_* linkedList_init() {
	linkedList_ *linkedList = safe_malloc(__func__, sizeof(linkedList_));
	linkedList->head = NULL;
	linkedList->size = 0;
	if (pthread_mutex_init(&linkedList->lock, NULL)) {
		ERR("linkedList_init(): Failed to initialize mutex\n");
		exit(EXIT_FAILURE);
	}

	return linkedList;
}

void linkedList_destroy(linkedList_ *linkedList) {
	node_ *node = linkedList->head, *temp;
	while (node != NULL) {
		temp = node->next;
		free(node);
		node = temp;
	}
	if (pthread_mutex_destroy(&linkedList->lock)) {
		ERR("linkedList_destroy(): Failed to destroy mutex\n");
		exit(EXIT_FAILURE);
	}

	free(linkedList);
}

/* ================================== HASHTABLE ================================= */


hashTable_* hashTable_init() {
	hashTable_ *hashTable = safe_malloc(__func__, sizeof(hashTable_));
	hashTable->size = 0;
	hashTable->capacity = 13;
	linkedList_ **table = hashTable->table = safe_malloc(__func__, sizeof(linkedList_ *)*13;
	for (int i = 0; i < 13; ++i)
		table[i] = NULL;

	return hashTable;
}

int getIndex(int size, char *key) {
	int hash;
	for (int i = 0; i < strlen(key); ++i)
		hash = HASH*31 + key[i];

	return hash%size;
}

int findNextPrime(size_t num) {
	if(num < 2) return num;

	int isPrime = 0;
	while (!isPrime) {
		isPrime = 1;
		for (int i = 2; i < num/2; ++i) {
			if (num % i == 0) {
				isPrime = 0;
				++num;
				break;
			}
		}
	}

	return num;
}

void resizeTable(hashTable_ *hashTable) { // TODO need to make hashTable lock
	int size = sizeof(linkedList_ *)*findNextPrime(hashTable->size*2);
	linkedList_ **newTable = safe_malloc(__func__, size);
	for (int i = 0; i < size; ++i)
		table[i] = NULL;

	linkedList **table = hashTable->table;
	for(int i = 0; i < hashTable->size; ++i) {
		if(table[i] != NULL) {
			node_ *node = table[i]->head;
			while(node != NULL) {
				node_insert(newTable, )
			}
		}
	}

	free(hashTable->table);
	hashTable->table = newTable;
}

void insertData(hashTable_ *hashTable, char *key, char *data) {
	int index = getIndex(hashTable->size, key), size = hashTable->size;
	if ((size+1) >= hashTable->capacity*0.8) {
		resizeTable(hashTable);
		index = getIndex(hashTable->size, key);
	}
	linkedList_ **table = hashTable->table;
	if (table[index] == NULL) {
		table[index] = linkedList_init();
		node_insert(table[index], key, data);
	} else {
		// TODO
	}
}

char* getData(hashTable_ *hashTable, char *key) {
	pthread_mutex_lock(&linkedList->lock);
	linkedList_ *linkedList = hashTable->table[getIndex(hashTable->size, key)];

	char *data = NULL;
	node_ *node = linkedList->head;

	if (node == NULL)
		return data;
	else {
		int retval;
		while (node != NULL && (retval = strcmp(key, node->key)))
			node = node->next;

		if (!retval)
			data = node->data;
	}

	pthread_mutex_unlock(&linkedList->lock);

	return data;
}

int removeData(hashTable_ *hashTable, char * key) {
	node_ *node, *prev = hashTable->table[getIndex(hashTable->size, key)]->head;

	if (node == NULL)
		return 1;

	while (node != NULL && strcmp(key, node->key)) {
		prev = node;
		node = node->next;
	}

	if (prev == NULL)
		linkedList->head = node->next;
	else
		prev->next = node->next;

	free(node);
	return 0;
}

void hashTable_destroy(hashTable_ *hashTable) {
	linkedList_ **table = hashTable->table;
	for (int i = 0; i < hashTable->capacity; ++i) {
		if (table[i] != NULL)
			linkedList_destroy(table[i]);
	}
	free(hashTable);
}