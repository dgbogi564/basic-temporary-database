#include "database.h"

#include <string.h>
#include <unistd.h>

#include "macros.h"

#ifndef HASH
#define HASH 3
#endif

// TODO
//  Make sure certain functions return an error code.
//  If any function returns NULL or 1 and send the appropriate error code to the client and server the connection if needed.


/* ============================ LINKEDLIST & NODES =============================== */

node_ *node_init(char *key, char *data)
{
	node_ *node = safe_malloc(__func__, sizeof(node_));
	node->key = key;
	node->data = data;
	node->next = NULL;
	return node;
}

linkedList_ *linkedList_init()
{
	linkedList_ *linkedList = safe_malloc(__func__, sizeof(linkedList_));
	linkedList->size = 0;
	linkedList->head = NULL;
	linkedList->rear = NULL;
	if (pthread_mutex_init(&linkedList->lock, NULL)) {
		eprintf("linkedList_init(): Failed to initialize mutex\n");
		exit(EXIT_FAILURE);
	}
	return linkedList;
}

void node_insert(linkedList_ *linkedList, node_ *newNode)
{
	pthread_mutex_lock(&linkedList->lock);

	node_ *inFront = linkedList->head, *prev = NULL;
	int retval;

	// Find the two nodes the new node should be placed between.
	while (inFront != NULL && (retval = strcmp(newNode->key, inFront->key)) > 0) {
		prev = inFront;
		inFront = inFront->next;
	}

	if (prev == NULL) {
		// The list is empty or the new node is smaller than any node in the list
		newNode->next = linkedList->head;
		linkedList->head = newNode;
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
		} else
			// The new node is larger than any node in the list
			linkedList->rear = linkedList->rear->next = newNode;
	}

	pthread_mutex_unlock(&linkedList->lock);
}

void linkedList_destroy(linkedList_ *linkedList)
{
	node_ *node = linkedList->head, *temp;
	while (node != NULL) {
		temp = node->next;
		free(node);
		node = temp;
	}
	if (pthread_mutex_destroy(&linkedList->lock)) {
		eprintf("linkedList_destroy(): Failed to destroy mutex\n");
		exit(EXIT_FAILURE);
	}
	free(linkedList);
}

/* ================================== HASHTABLE ================================= */

hashTable_ *hashTable_init()
{
	hashTable_ *hashTable = safe_malloc(__func__, sizeof(hashTable_));
	hashTable->size = 0;
	hashTable->capacity = 13;
	hashTable->num_working = 0;
	hashTable->wr_ready = true;
	hashTable->table = safe_malloc(__func__, sizeof(linkedList_ *) * 13);
	for (int i = 0; i < 13; ++i)
		hashTable->table[i] = NULL;

	return hashTable;
}

int hash(int size, char *key)
{
	int hash;
	for (int i = 0; i < strlen(key); ++i)
		hash = HASH * 31 + key[i];

	return hash % size;
}

int nextPrime(int n)
{
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

void moveData(hashTable_ *hashTable, node_ *node)
{
	int index = hash(hashTable->capacity, node->key);
	linkedList_ **table = hashTable->table;
	if (table[index] == NULL)
		table[index] = linkedList_init();

	node_insert(table[index], node);
}

void resizeTable(hashTable_ *hashTable)
{
	int newCap = nextPrime(hashTable->size * 2);

	// Tell all threads to finish and pause work on the hashtable
	hashTable->wr_ready = false;
	// Wait until all threads have finished their work
	while (hashTable->num_working)
		nanosleep((const struct timespec[]) {{0, 1000000000L}}, NULL);

	linkedList_ **oldTable = hashTable->table, **newTable;
	int oldCap = hashTable->capacity;

	// Allocate space for new table and set entire array to NULL
	hashTable->capacity = newCap;
	newTable = hashTable->table = safe_malloc(__func__, sizeof(linkedList_ *) * newCap);
	for (int i = 0; i < newCap; ++i)
		newTable[i] = NULL;


	node_ *node, *temp;
	for (int i = 0; i < oldCap; ++i) {
		// Only move node's in allocated linked lists
		if (oldTable[i] != NULL) {
			node = oldTable[i]->head;
			while (node != NULL) {
				// Re-insert all nodes into new table
				temp = node->next;
				node->next = NULL;
				moveData(hashTable, node);
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
	hashTable->wr_ready = true;
}

void insertData(hashTable_ *hashTable, char *key, char *data)
{
	while(!hashTable->wr_ready) { }
	hashTable->num_working++;

	// Check if the hashtable has reached 80% capacity
	if (hashTable->size >= hashTable->capacity * 0.8)
		resizeTable(hashTable);

	int index = hash(hashTable->size, key);
	linkedList_ **table = hashTable->table;

	// Check if there's a collision
	if (table[index] == NULL)
		table[index] = linkedList_init();

	// Insert data
	node_insert(table[index], node_init(key, data));

	hashTable->num_working--;
}

char *getData(hashTable_ *hashTable, char *key)
{
	while(!hashTable->wr_ready) { }
	hashTable->num_working++;

	char *data = NULL;
	node_ *node;
	int retval = 1;
	linkedList_ *linkedList = hashTable->table[hash(hashTable->capacity, key)];
	// If the list is not empty.
	if (linkedList != NULL) {

		pthread_mutex_lock(&linkedList->lock);

		node = linkedList->head;
		// Find the node that contains the data
		while (node != NULL && (retval = strcmp(key, node->key)))
			node = node->next;

		// If the key exists in the hash table
		if (!retval)
			data = node->data;

		pthread_mutex_unlock(&linkedList->lock);
	}

	hashTable->num_working--;
	return data;
}

int removeData(hashTable_ *hashTable, char *key)
{
	while(!hashTable->wr_ready) { }
	hashTable->num_working++;

	int index = hash(hashTable->capacity, key);
	linkedList_ *linkedList = hashTable->table[index];
	node_ *node = linkedList->head, *prev, *temp;
	// If linked list doesn't exist
	if (node == NULL)
		return 1;

	pthread_mutex_lock(&linkedList->lock);
	// Find the node to be deleted and its previous node.
	while (node != NULL && !strcmp(key, node->key)) {
		prev = node;
		node = node->next;
	}
	if (prev == NULL) {
		// If that's the only node in the list
		if (node->next == NULL)
			// Delete list since it will be empty
			linkedList_destroy(linkedList);
		else {
			// Remove first node in list
			linkedList->head = node->next;
			free(node);
		}
	} else {
		// If node doesn't exist
		if (node == NULL)
			return 1;

		// Remove node
		prev->next = node->next;
		free(node);
	}
	pthread_mutex_unlock(&linkedList->lock);

	hashTable->num_working--;
	return 0;
}

void hashTable_destroy(hashTable_ *hashTable)
{
	linkedList_ **table = hashTable->table;
	for (int i = 0; i < hashTable->capacity; ++i) {
		if (table[i] != NULL)
			linkedList_destroy(table[i]);
	}
	free(hashTable);
}