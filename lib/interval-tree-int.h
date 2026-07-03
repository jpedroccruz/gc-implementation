#ifndef INTERVAL_TREE_INT_H
#define INTERVAL_TREE_INT_H


typedef struct Interval {
  int low, high;
} Interval;

typedef struct Node {
  Interval *i;
  int max;
  int height;

  struct Node *right, *left;
} Node;

Node *createNode(Interval *i);
int getMaxValue(int x, int y);
void updateMax(Node *node);
int getNodeHeight(Node *node);
int getBalanceFactor(Node *node);
void rightRotation(Node **n);
void leftRotation(Node **n);
void insert(Node **root, Interval *i);
void printTree(Node *root, int level);
Node *findInterval(Node *node, Interval *i);
Node *findPoint(Node *node, int point);
Node *getMinValue(Node *node);
void removeInterval(Node **node, Interval *i);

#endif