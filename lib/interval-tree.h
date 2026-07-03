#ifndef INTERVAL_TREE_INT_H
#define INTERVAL_TREE_INT_H

#include <stdint.h>

typedef struct Interval {
  uintptr_t low, high;
} Interval;

typedef struct Node {
  Interval i;
  uintptr_t max;
  int height;

  struct Node *right, *left;
} Node;

Node *createNode(Interval i);
uintptr_t getMaxValue(uintptr_t x, uintptr_t y);
void updateMax(Node *node);
int getNodeHeight(Node *node);
int getBalanceFactor(Node *node);
void rightRotation(Node **n);
void leftRotation(Node **n);
void insert(Node **root, Interval i);
void printTree(Node *root, int level);
Node *findInterval(Node *node, Interval i);
Node *findPoint(Node *node, uintptr_t point);
Node *getMinValue(Node *node);
void removeInterval(Node **node, Interval i);

#endif