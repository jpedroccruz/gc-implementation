#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

typedef struct Heap {
  void   *base;       
  size_t  capacity;   
  size_t  offset;
} Heap;

Heap *createHeap(size_t size);
void *allocHeap(Heap *h, size_t size);
void resetHeap(Heap *h);
void destroyHeap(Heap *h);

#endif