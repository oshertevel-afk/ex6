#include <stdlib.h>
#include "bst.h"

static BSTNode* createNode(void* data);

static BSTNode* createNode(void* data) {
	BSTNode* node = (BSTNode*)malloc(sizeof(BSTNode));
	if (node == NULL) {
		exit(1);
	}
	node->data = data;
	node->left = NULL;
	node->right = NULL;
	return node;
}

BST* createBST(int (*cmp)(void*, void*), void (*print)(void*), void (*freeData)(void*)) {
	BST* tree = (BST*)malloc(sizeof(BST));
	if (tree == NULL) {
		exit(1);
	}
	tree->root = NULL;
	tree->compare = cmp;
	tree->print = print;
	tree->freeData = freeData;
	return tree;
}

BSTNode* bstInsert(BSTNode* root, void* data, int (*cmp)(void*, void*)) {
	int c = 0;

	if (root == NULL) {
		return createNode(data);
	}

	c = cmp(data, root->data);
	if (c < 0) {
		root->left = bstInsert(root->left, data, cmp);
	} else if (c > 0) {
		root->right = bstInsert(root->right, data, cmp);
	}

	return root;
}

void* bstFind(BSTNode* root, void* data, int (*cmp)(void*, void*)) {
	int c = 0;

	if (root == NULL) {
		return NULL;
	}

	c = cmp(data, root->data);
	if (c == 0) {
		return root->data;
	}
	if (c < 0) {
		return bstFind(root->left, data, cmp);
	}
	return bstFind(root->right, data, cmp);
}

void bstInorder(BSTNode* root, void (*print)(void*)) {
	if (root == NULL) {
		return;
	}
	bstInorder(root->left, print);
	print(root->data);
	bstInorder(root->right, print);
}

void bstPreorder(BSTNode* root, void (*print)(void*)) {
	if (root == NULL) {
		return;
	}
	print(root->data);
	bstPreorder(root->left, print);
	bstPreorder(root->right, print);
}

void bstPostorder(BSTNode* root, void (*print)(void*)) {
	if (root == NULL) {
		return;
	}
	bstPostorder(root->left, print);
	bstPostorder(root->right, print);
	print(root->data);
}

void bstFree(BSTNode* root, void (*freeData)(void*)) {
	if (root == NULL) {
		return;
	}
	bstFree(root->left, freeData);
	bstFree(root->right, freeData);
	freeData(root->data);
	free(root);
}
