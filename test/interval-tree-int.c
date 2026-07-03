#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../lib/interval-tree-int.h"

static int checkInvariants(Node *node, int *ok) {
  if (node == NULL) return -1;

  int leftHeight  = checkInvariants(node->left, ok);
  int rightHeight = checkInvariants(node->right, ok);

  int expectedHeight = 1 + ((leftHeight > rightHeight) ? leftHeight : rightHeight);
  if (node->height != expectedHeight) {
    printf("[INVARIANTE QUEBRADA] no [%d,%d]: height=%d, esperado=%d\n",
           node->i->low, node->i->high, node->height, expectedHeight);
    *ok = 0;
  }

  int balance = leftHeight - rightHeight;
  if (balance < -1 || balance > 1) {
    printf("[INVARIANTE QUEBRADA] no [%d,%d]: fator de balanceamento=%d (fora de [-1,1])\n",
           node->i->low, node->i->high, balance);
    *ok = 0;
  }

  int expectedMax = node->i->high;
  if (node->left  != NULL && node->left->max  > expectedMax) expectedMax = node->left->max;
  if (node->right != NULL && node->right->max > expectedMax) expectedMax = node->right->max;
  if (node->max != expectedMax) {
    printf("[INVARIANTE QUEBRADA] no [%d,%d]: max=%d, esperado=%d\n",
           node->i->low, node->i->high, node->max, expectedMax);
    *ok = 0;
  }

  if (node->left != NULL && node->left->i->low >= node->i->low) {
    printf("[INVARIANTE QUEBRADA] no [%d,%d]: filho esquerdo com low >= pai\n",
           node->i->low, node->i->high);
    *ok = 0;
  }
  if (node->right != NULL && node->right->i->low < node->i->low) {
    printf("[INVARIANTE QUEBRADA] no [%d,%d]: filho direito com low < pai\n",
           node->i->low, node->i->high);
    *ok = 0;
  }

  return expectedHeight;
}

static void assertValidTree(Node *root, const char *label) {
  int ok = 1;
  checkInvariants(root, &ok);
  if (!ok) {
    printf("FALHA: árvore inválida após '%s'\n", label);
  }
  assert(ok);
}

static int countNodes(Node *node) {
  if (node == NULL) return 0;
  return 1 + countNodes(node->left) + countNodes(node->right);
}

static void it_should_insert_and_keep_invariants(void) {
  Node *root = NULL;
  Interval i1 = {1, 2};
  Interval i2 = {2, 4};
  Interval i3 = {3, 6};
  Interval i4 = {4, 8};

  insert(&root, &i1);
  assertValidTree(root, "insert i1");
  insert(&root, &i2);
  assertValidTree(root, "insert i2");
  insert(&root, &i3);
  assertValidTree(root, "insert i3");
  insert(&root, &i4);
  assertValidTree(root, "insert i4");

  assert(countNodes(root) == 4);
  printf("it should insert and keep invariants: OK\n");
}

static void it_should_rotate_LL_on_insert(void) {
  Node *root = NULL;
  Interval a = {30, 30}, b = {20, 20}, c = {10, 10};
  insert(&root, &a);
  insert(&root, &b);
  insert(&root, &c);
  assertValidTree(root, "LL rotation");
  assert(root->i->low == 20);
  printf("it should rotate LL oninsert: OK\n");
}

static void it_should_rotate_RR_on_insert(void) {
  Node *root = NULL;
  Interval a = {10, 10}, b = {20, 20}, c = {30, 30};
  insert(&root, &a);
  insert(&root, &b);
  insert(&root, &c);
  assertValidTree(root, "RR rotation");
  assert(root->i->low == 20);
  printf("it should rotate RR on insert: OK\n");
}

static void it_should_rotate_LR_on_insert(void) {
  Node *root = NULL;
  Interval a = {30, 30}, b = {10, 10}, c = {20, 20};
  insert(&root, &a);
  insert(&root, &b);
  insert(&root, &c);
  assertValidTree(root, "LR rotation");
  assert(root->i->low == 20);
  printf("it should rotate LR on insert: OK\n");
}

static void it_should_rotate_RL_on_insert(void) {
  Node *root = NULL;
  Interval a = {10, 10}, b = {30, 30}, c = {20, 20};
  insert(&root, &a);
  insert(&root, &b);
  insert(&root, &c);
  assertValidTree(root, "RL rotation");
  assert(root->i->low == 20);
  printf("it should rotate RL on insert: OK\n");
}

static void it_should_find_point_inside_interval(void) {
  Node *root = NULL;
  Interval i1 = {1, 2};
  Interval i2 = {5, 10};
  Interval i3 = {15, 20};

  insert(&root, &i1);
  insert(&root, &i2);
  insert(&root, &i3);

  Node *found = findPoint(root, 7);
  assert(found != NULL);
  assert(found->i->low == 5 && found->i->high == 10);

  found = findPoint(root, 5);
  assert(found != NULL && found->i->low == 5);

  found = findPoint(root, 10);
  assert(found != NULL && found->i->low == 5);

  found = findPoint(root, 12);
  assert(found == NULL);

  printf("it should find point inside interval: OK\n");
}

static void it_should_find_overlapping_interval(void) {
  Node *root = NULL;
  Interval i1 = {1, 5};
  Interval i2 = {10, 15};

  insert(&root, &i1);
  insert(&root, &i2);

  Interval queryOverlap = {4, 6};
  Node *found = findInterval(root, &queryOverlap);
  assert(found != NULL);

  Interval queryNone = {6, 9};
  found = findInterval(root, &queryNone);
  assert(found == NULL);

  printf("it should find overlapping interval: OK\n");
}

static void it_should_remove_a_leaf(void) {
  Node *root = NULL;
  Interval i1 = {10, 10};
  Interval i2 = {5, 5};
  Interval i3 = {15, 15};

  insert(&root, &i1);
  insert(&root, &i2);
  insert(&root, &i3);
  assert(countNodes(root) == 3);

  removeInterval(&root, &i2);
  assertValidTree(root, "remove leaf");
  assert(countNodes(root) == 2);
  assert(findPoint(root, 5) == NULL);

  printf("it should remove a leaf: OK\n");
}

static void it_should_remove_a_node_with_one_child(void) {
  Node *root = NULL;
  Interval i1 = {10, 10};
  Interval i2 = {5, 5};
  Interval i3 = {1, 1};

  insert(&root, &i1);
  insert(&root, &i2);
  insert(&root, &i3);
  assertValidTree(root, "setup remove one child");
  assert(countNodes(root) == 3);

  removeInterval(&root, &i1);
  assertValidTree(root, "remove um filho");
  assert(countNodes(root) == 2);

  printf("it should remove a node with one child: OK\n");
}

static void it_should_remove_a_node_with_two_children(void) {
  Node *root = NULL;
  Interval i1 = {10, 10};
  Interval i2 = {5, 5};
  Interval i3 = {15, 15};
  Interval i4 = {12, 12};
  Interval i5 = {20, 20};

  insert(&root, &i1);
  insert(&root, &i2);
  insert(&root, &i3);
  insert(&root, &i4);
  insert(&root, &i5);
  assertValidTree(root, "setup remove two children");
  assert(countNodes(root) == 5);

  removeInterval(&root, &i1);
  assertValidTree(root, "remove no com dois filhos");
  assert(countNodes(root) == 4);
  assert(findPoint(root, 10) == NULL);

  printf("it should remove a node with two children: OK\n");
}

static void it_should_remove_root_repeatedly_until_empty(void) {
  Node *root = NULL;
  Interval intervals[7] = {
    {40,40}, {20,20}, {60,60}, {10,10}, {30,30}, {50,50}, {70,70}
  };

  for (int i = 0; i < 7; i++) {
    insert(&root, &intervals[i]);
  }
  assertValidTree(root, "setup remove root until empty");
  assert(countNodes(root) == 7);

  for (int i = 0; i < 7; i++) {
    assert(root != NULL);
    Interval toRemove = *(root->i);
    removeInterval(&root, &toRemove);
    char label[64];
    snprintf(label, sizeof(label), "sequential remove #%d", i + 1);
    assertValidTree(root, label);
    assert(countNodes(root) == 6 - i);
  }

  assert(root == NULL);
  printf("it should remove root repeatedly until empty: OK\n");
}

int main(void) {
  it_should_insert_and_keep_invariants();
  it_should_rotate_LL_on_insert();
  it_should_rotate_RR_on_insert();
  it_should_rotate_LR_on_insert();
  it_should_rotate_RL_on_insert();
  it_should_find_point_inside_interval();
  it_should_find_overlapping_interval();
  it_should_remove_a_leaf();
  it_should_remove_a_node_with_one_child();
  it_should_remove_a_node_with_two_children();
  it_should_remove_root_repeatedly_until_empty();

  printf("\nAll interval tree tests passed.\n");
  return 0;
}