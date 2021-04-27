#include "bst.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

bstNode_ *bstNode_init(char *key, char *value) {
	bstNode_ *bstNode = MALLOC(sizeof(bstNode_ *));
	bstNode->key = key;
	bstNode->value = value;
	bstNode->left = NULL;
	bstNode->right = NULL;
	return bstNode;
}

bst_ *bst_init() {
	bst_ *bst = MALLOC("bst_init", sizeof(bst_ *));
	if (pthread_mutex_init(&bst->lock)) {
		ERR("bst_init(): Failed to initialize lock\n");
	}
	bst->root = NULL;
	++bst->size;
	return bst;
}
void bst_rebalance(bstNode_ *bstNode) {

}

int bstNode_insert(bst_ *bst, char *key, char *value) {
	if (bst == NULL)
		return 1;

	if (bst->root == NULL)
		bst->root = bstNode_init(key);
	else {
		bstNode_ *bstNode = bst->root, *prev = NULL;

		int compare;
		while (bstNode != NULL) {
			prev = bstNode;
			if ((compare = strcmp(key, bstNode->key)) < 0)
				bstNode = bstNode->left;
			else if (compare > 0)
				bstNode = bstNode->right;
			else {
				bstNode->value = value;
				return 0;
			}
		}

		if (strcmp(key, prev->key) < 0)
			prev->left = bstNode_init(key, value);
		else
			prev->right = bstNode_init(key, value);
	}

	++bst->size;

	return 0;
}

char* bstNode_peek(bst_ *bst, char *key) {

}

int bstNode_remove(bst_ *bst, char *key) {
	if (bst == NULL || bst->root == NULL)
		return 1;

	bstNode_ *bstNode = bst->root, *prev = NULL;
	int compare;
	while (bstNode != NULL && !strcmp(key, bstNode->key)) {
		prev = bstNode;
		if ((compare = strcmp(key, bstNode->key)) < 0)
			bstNode = bstNode->left;
		else if (compare > 0)
			bstNode = bstNode->right;
	}

	if (bstNode == NULL)
		return 2;

	if(bstNode->left == NULL || bstNode->right == NULL) {
		newBstNode;

		if (bstNode->left == NULL)
			newBstNode = bstNode->left;
		else
			newBstNode = bstNode->right;

		if (prev == NULL)
			bst->root = newBstNode;
		else if (prev->left == bstNode)
			prev->left = newBstNode;
		else
			prev->right = newBstNode;

		free(bstNode);
		--bst->size;

		return 0;
	}

	bstNode_ *temp = bstNode;
	while(temp->left != NULL) {
		//
	}

	--bst->size;

	return 0;
}

void bstNode_destroy(bstNode_ *bstNode) {
	free(bstNode);
}

void bst_destroy(bst_ *bst) {
	bstNode_ *bstNode = bst->root, *temp;
	while (bstNode != NULL) {
		temp = bstNode->next;
		free(bstNode);
		bstNode = temp;
	}
	if (pthread_mutex_destroy(&bst->lock)) {
		ERR("bst_init(): Failed to destroy lock\n");
	}
	free(bst);
}