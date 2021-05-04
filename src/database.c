#include "database.h"
#include "server.h"

#include <string.h>

#ifndef HASH
#define HASH 3
#endif

// TODO
//  Make sure certain functions return an error code.
//  If any function returns NULL or 1 and send the appropriate error code to the client and server the connection if needed.


/* ============================ LINKEDLIST & NODES =============================== */

node_ *node_init(char *key, char *data) {
	node_ *node = safe_malloc(__func__, sizeof(node_));
	node->key = key;
	node->data = data;
	node->next = NULL;
	return node;
}

linkedlist_ *linkedlist_init() {
	linkedlist_ *linkedlist = safe_malloc(__func__, sizeof(linkedlist_));
	linkedlist->size = 0;
	linkedlist->head = NULL;
	linkedlist->rear = NULL;
	if (pthread_mutex_init(&linkedlist->lock, NULL)) {
		eprintf("linkedlist_init(): Failed to initialize mutex\n");
		fprintf(f_send, "ERR SRV\n");
		exit(EXIT_FAILURE);
	}
	return linkedlist;
}

void node_insert(linkedlist_ *linkedlist, node_ *newNode) {
	pthread_mutex_lock(&linkedlist->lock);

	node_ *inFront = linkedlist->head, *prev = NULL;
	int retval;

	// Find the two nodes the new node should be placed between.
	while (inFront != NULL && (retval = strcmp(newNode->key, inFront->key)) > 0) {
		prev = inFront;
		inFront = inFront->next;
	}

	if (prev == NULL) {
		// The list is empty or the new node is smaller than any node in the list
		newNode->next = linkedlist->head;
		linkedlist->head = newNode;
	} else {
		if (inFront != NULL) {
			if (retval) {
				// The new node belongs in the middle of the list
				prev->next = newNode;
				newNode->next = inFront;
			} else {
				// The new node already exists and just needs to update it's data
				inFront->data = newNode->data;
				free(newNode);
			}
		} else {
			// The new node is larger than any node in the list
			linkedlist->rear = linkedlist->rear->next = newNode;
		}
	}

	pthread_mutex_unlock(&linkedlist->lock);
}

void linkedlist_destroy(linkedlist_ *linkedlist) {
	node_ *node = linkedlist->head, *temp;
	while (node != NULL) {
		temp = node->next;
		free(node);
		node = temp;
	}
	if (pthread_mutex_destroy(&linkedlist->lock)) {
		eprintf("linkedlist_destroy(): Failed to destroy mutex\n");
		exit(EXIT_FAILURE);
	}
	free(linkedlist);
}

/* ================================== HASHTABLE ================================= */

hashtable_ *hashtable_init() {
	hashtable_ *hashtable = safe_malloc(__func__, sizeof(hashtable_));
	hashtable->size = 0;
	hashtable->capacity = 13;
	hashtable->num_working = 0;
	hashtable->wr_ready = true;
	hashtable->table = safe_malloc(__func__, sizeof(linkedlist_ *) * 13);
	for (int i = 0; i < 13; ++i) {
		hashtable->table[i] = NULL;
	}
	return hashtable;
}

int hash(int size, char *key) {
	int hash;
	for (int i = 0; i < strlen(key); ++i) {
		hash = HASH * 31 + key[i];
	}
	return hash % size;
}

int nextPrime(int n) {
	if (n < 2) return n;

	bool isPrime = false;
	while (!isPrime) {
		isPrime = true;
		for (int i = 2; i < n / 2; ++i) {
			if (n % i == 0) {
				isPrime = 0;
				n++;
				break;
			}
		}
	}

	return n;
}

void moveData(hashtable_ *hashtable, node_ *node) {
	int index = hash(hashtable->capacity, node->key);
	linkedlist_ **table = hashtable->table;
	if (table[index] == NULL) {
		table[index] = linkedlist_init();
	}
	node_insert(table[index], node);
}

void resizeTable(hashtable_ *hashtable) {
	int newCap = nextPrime(hashtable->size * 2);

	// Tell all threads to finish and pause work on the hashtable
	hashtable->wr_ready = false;
	// Wait until all threads have finished their work
	while (hashtable->num_working) {
		// Sleep for 0.1 second(s).
		nanosleep((const struct timespec[]) {{0, 100000000L}}, NULL);
	}
	linkedlist_ **oldTable = hashtable->table, **newTable;
	int oldCap = hashtable->capacity;

	// Allocate space for new table and set entire array to NULL
	hashtable->capacity = newCap;
	newTable = hashtable->table = safe_malloc(__func__, sizeof(linkedlist_ *) * newCap);
	for (int i = 0; i < newCap; ++i) {
		newTable[i] = NULL;
	}


	node_ *node, *temp;
	for (int i = 0; i < oldCap; ++i) {
		// Only move node's in allocated linked lists
		if (oldTable[i] != NULL) {
			node = oldTable[i]->head;
			while (node != NULL) {
				// Re-insert all nodes into new table
				temp = node->next;
				moveData(hashtable, node);
				node->next = NULL;
				node = temp;
			}
			// Free linked list
			if (pthread_mutex_destroy(&oldTable[i]->lock)) {
				eprintf("resizeTable(): Failed to destroy mutex\n");
				exit(EXIT_FAILURE);
			}
			free(oldTable[i]);
		}
	}
	// Free old table and tell all threads to continue their work
	free(oldTable);
	hashtable->wr_ready = true;
}

void insertData(hashtable_ *hashtable, char *key, char *data) {
	while (!hashtable->wr_ready) {
		// Sleep for 0.1 second(s)
		nanosleep((const struct timespec[]) {{0, 100000000L}}, NULL);
	}

	hashtable->num_working++;

	// Check if the hashtable has reached 80% capacity
	if (hashtable->size >= hashtable->capacity * 0.8) {
		resizeTable(hashtable);
	}

	int index = hash(hashtable->size, key);
	linkedlist_ **table = hashtable->table;

	// Check if there's a collision
	if (table[index] == NULL) {
		table[index] = linkedlist_init();
	}

	// Insert data
	node_insert(table[index], node_init(key, data));

	hashtable->num_working--;
}

char *getData(hashtable_ *hashtable, char *key) {
	while (!hashtable->wr_ready) {
		// Sleep for 0.1 second(s)
		nanosleep((const struct timespec[]) {{0, 100000000L}}, NULL);
	}

	hashtable->num_working++;

	char *data = NULL;
	node_ *node;
	int retval = 1;
	linkedlist_ *linkedlist = hashtable->table[hash(hashtable->capacity, key)];
	// If the list is not empty.
	if (linkedlist != NULL) {

		pthread_mutex_lock(&linkedlist->lock);

		node = linkedlist->head;
		// Find the node that contains the data
		while (node != NULL && (retval = strcmp(key, node->key)))
			node = node->next;

		// If the key exists in the hash table
		if (!retval) data = node->data;

		pthread_mutex_unlock(&linkedlist->lock);
	}

	hashtable->num_working--;
	return data;
}

char* delData(hashtable_ *hashtable, char *key) {
	char *data = NULL;

	while (!hashtable->wr_ready) {
		// Sleep for 0.1 second(s)
		nanosleep((const struct timespec[]) {{0, 100000000L}}, NULL);
	}

	hashtable->num_working++;

	int index = hash(hashtable->capacity, key);
	linkedlist_ *linkedlist = hashtable->table[index];
	// If linkedlist is NULL
	if (linkedlist == NULL) return NULL;
	node_ *node = linkedlist->head, *prev;
	// If the nodes are NULL
	if (node == NULL) return NULL;

	pthread_mutex_lock(&linkedlist->lock);
	// Find the node to be deleted and its previous node.
	while (node != NULL && !strcmp(key, node->key)) {
		prev = node;
		node = node->next;
	}
	if(node == NULL) return NULL;
	data = node->data;

	if (prev == NULL) {
		// If node is the only entry in the list
		if (node->next == NULL) {
			// Delete list since it's empty
			linkedlist_destroy(linkedlist);
		} else {
			// Remove the first node in list
			linkedlist->head = node->next;
			free(node);
		}
	} else {
		// Remove node from the list
		prev->next = node->next;
		free(node);
	}
	pthread_mutex_unlock(&linkedlist->lock);

	hashtable->num_working--;
	return data;
}

void table_destroy(linkedlist_ **table, int capacity) {
	for (int i = 0; i < capacity; ++i) {
		if (table[i] != NULL) {
			linkedlist_destroy(table[i]);
		}
	}
	free(table);
}

void hashtable_destroy(hashtable_ *hashtable) {
	table_destroy(hashtable->table, hashtable->capacity);
	free(hashtable);
}