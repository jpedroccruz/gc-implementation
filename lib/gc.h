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
} ObjHeader;

void gc_init(void);
void *gc_malloc(size_t size);

#endif