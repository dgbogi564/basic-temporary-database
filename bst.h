#ifndef P3_BST_H
#define P3_BST_H

#include <pthread.h>

typedef struct bstNode_ {
    char *key, *value;
    struct bstNode_ *left, *right;
} bstNode_;

typedef struct bst_ {
    int size;
    bstNode_ *root;
    pthread_mutex_t lock;
} bst_;

typedef struct stack_ {
	bstNode_ *bstNode;
	stack_ *next;
} stack_;


bstNode_ *bstNode_init(char *key, char *value);
bst_ *bst_init();
void bstNode_insert();
char* bstNode_peek(bst_ *bst, char *key);
void bstNode_remove(bst_ *bst, char *key);
void bstNode_destroy(bstNode_ *bstNode);
void bst_destroy(bst_ *bst);

#endif //P3_BST_H
