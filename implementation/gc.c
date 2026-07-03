#include "../lib/gc.h"
#include <stdlib.h>
#include <stdio.h>

#define YOUNG_GEN_SIZE (1024 * 1024)

static Heap *youngGeneration = NULL;
Node *global_root = NULL;

void gc_init(void) {
  if (youngGeneration == NULL) {
    youngGeneration = createHeap(YOUNG_GEN_SIZE);

    if (!youngGeneration) {
      perror("Failed to init GC Heap.");
      exit(1);
    }
  }
}

void *gc_malloc(size_t size) {
  if (size == 0) return NULL;

  if (!youngGeneration) gc_init();
  
  size_t totalSize = sizeof(ObjHeader) + size;

  void *mem = allocHeap(youngGeneration, totalSize);

  if (mem == NULL) {
    // gc_collector()
    return NULL;
  }

  ObjHeader *header = (ObjHeader *)mem;
  header->size = size;
  header->generation = 0;
  header->marked = 0;
  header->canary = 0xDEADC0DE;

  void *pointer = (char *)mem + sizeof(ObjHeader);

  uintptr_t low = (uintptr_t)mem;
  uintptr_t high = low + totalSize;
  Interval i = {low, high};

  insert(&global_root, i);

  return pointer;
}
