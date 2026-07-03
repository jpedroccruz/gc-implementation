#include "../lib/interval-tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

Node *createNode(Interval *i) {
  Node *new_node = malloc(sizeof(Node));

  new_node->i = i;
  new_node->left = NULL;
  new_node->right = NULL;
  new_node->max = i->high;
  new_node->height = 0;

  return new_node;
}

uintptr_t getMaxValue(uintptr_t x, uintptr_t y) {
  return (x > y) ? x : y; 
}

int getMaxHeight(int x, int y) {
  return (x > y) ? x : y; 
}

void updateMax(Node *node) {
  if (node == NULL) return;

  node->max = node->i->high;

  if (node->left != NULL && node->left->max > node->max) node->max = node->left->max;
  if (node->right != NULL && node->right->max > node->max) node->max = node->right->max;
}

int getNodeHeight(Node *node) {
  if (node == NULL) return -1;
  return node->height;
}

int getBalanceFactor(Node *node) {
  if (node == NULL) return 0;
  return getNodeHeight(node->left) - getNodeHeight(node->right);
}

void rightRotation(Node **n) {
  Node *nl = (*n)->left;
  Node *nlr = (*n)->left->right;

  nl->right = *n;
  (*n)->left = nlr;

  (*n)->height = 1 + getMaxHeight(getNodeHeight((*n)->left), getNodeHeight((*n)->right));
  nl->height = 1 + getMaxHeight(getNodeHeight(nl->left), getNodeHeight(nl->right));

  updateMax(*n);
  updateMax(nl);

  *n = nl;
}

void leftRotation(Node **n) {
  Node *nr = (*n)->right;
  Node *nrl = (*n)->right->left;

  nr->left = *n;
  (*n)->right = nrl;

  (*n)->height = 1 + getMaxHeight(getNodeHeight((*n)->left), getNodeHeight((*n)->right));
  nr->height = 1 + getMaxHeight(getNodeHeight(nr->left), getNodeHeight(nr->right));

  updateMax(*n);
  updateMax(nr);

  *n = nr;
}

void insert(Node **root, Interval *i) {
  if (*root == NULL) {
    *root = createNode(i);
    return;
  }

  if (i->low < (*root)->i->low) insert(&(*root)->left, i);
  else insert(&(*root)->right, i);

  (*root)->height = 1 + getMaxHeight(getNodeHeight((*root)->left), getNodeHeight((*root)->right));
  updateMax(*root);

  int balanceFactor = getBalanceFactor(*root);

  // LL
  if (balanceFactor > 1 && i->low < (*root)->left->i->low) {
    rightRotation(root);
    return;
  }

  // RR
  else if (balanceFactor < -1 && i->low >= (*root)->right->i->low) {
    leftRotation(root);
    return;
  }

  // LR
  else if (balanceFactor > 1 && i->low >= (*root)->left->i->low) {
    leftRotation(&(*root)->left);
    rightRotation(root);
    return;
  }

  // RL
  else if (balanceFactor < -1 && i->low < (*root)->right->i->low) {
    rightRotation(&(*root)->right);
    leftRotation(root);
    return;
  }
}

void printTree(Node *root, int level) {
  if (root != NULL) {
    printTree(root->right, level + 1);
    for (int i = 0; i < level * 5; i++) printf(" ");
    printf("[%ju, %ju] max=%ju\n\n", root->i->low, root->i->high, root->max);
    printTree(root->left, level + 1);
  }
}

Node *findInterval(Node *node, Interval *i) {
  if (node == NULL) return NULL;
  if (node->i->high >= i->low && node->i->low <= i->high) return node;
  if (node->left != NULL && node->left->max >= i->low) return findInterval(node->left, i);
  else return findInterval(node->right, i);
}

Node *findPoint(Node *node, uintptr_t point) {
  if (node == NULL) return NULL;
  if (node->i->low <= point && node->i->high >= point) return node;
  if (node->left != NULL && node->left->max >= point) return findPoint(node->left, point);
  else return findPoint(node->right, point);
}

Node *getMinValue(Node *node) {
  while (node->left != NULL) node = node->left;
  return node;
}

void removeInterval(Node **node, Interval *i) {
  if (*node == NULL) return;
  
  if ((*node)->i->low > i->low) removeInterval(&(*node)->left, i);
  else if ((*node)->i->low < i->low) removeInterval(&(*node)->right, i);
  else {
    if ((*node)->left == NULL && (*node)->right == NULL) {
      free(*node);
      *node = NULL;
    } else if ((*node)->left == NULL) {
      Node *temp = *node;
      *node = (*node)->right;
      free(temp);
    } else if ((*node)->right == NULL) {
      Node *temp = *node;
      *node = (*node)->left;
      free(temp);
    } else {
      Node *sucessor = getMinValue((*node)->right);
      (*node)->i = sucessor->i;
      removeInterval(&(*node)->right, sucessor->i);
    }
  }

  if (*node == NULL) return;

  (*node)->height = 1 + getMaxHeight(getNodeHeight((*node)->left), getNodeHeight((*node)->right));
  updateMax(*node);

  int balanceFactor = getBalanceFactor(*node);

  // LL
  if (balanceFactor > 1 && getBalanceFactor((*node)->left) >= 0) {
    rightRotation(node);
    return;
  }

  // RR
  else if (balanceFactor < -1 && getBalanceFactor((*node)->right) <= 0) {
    leftRotation(node);
    return;
  }

  // LR
  else if (balanceFactor > 1 && getBalanceFactor((*node)->left) < 0) {
    leftRotation(&(*node)->left);
    rightRotation(node);
    return;
  }

  // RL
  else if (balanceFactor < -1 && getBalanceFactor((*node)->right) > 0) {
    rightRotation(&(*node)->right);
    leftRotation(node);
    return;
  }
}