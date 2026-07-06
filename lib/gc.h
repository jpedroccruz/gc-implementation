#ifndef GC_H
#define GC_H

#include "./heap.h"
#include "./interval-tree.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  size_t size;
  int generation;
  int marked;
  uintptr_t canary;
  void *forwarded;
} ObjHeader;

typedef struct FreeBlock {
  uintptr_t address;
  size_t size;
  struct FreeBlock *next;
} FreeBlock;

extern Heap *youngGeneration;
extern Interval youngIntervals[];
extern int youngCount;

extern Heap *oldGeneration;
extern Interval oldDeadIntervals[];
extern int oldDeadCount;

extern void *dirtyPages[];
extern int dirtyPagesCount;

extern FreeBlock freeBlockPool[];
extern FreeBlock *availableBlocks;
extern FreeBlock *oldFreeList;

extern Node *global_root;
extern void *stack_base;

void reset_all_marks(Node *root);
void gc_init(void *stack_bottom);
void *gc_malloc(size_t size);

#endif