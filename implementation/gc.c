#include "../lib/gc.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#define YOUNG_GEN_SIZE (1024 * 1024)
#define MAX_GARBAGE 1024

static Heap *youngGeneration = NULL;
Node *global_root = NULL;
void *stack_base = NULL;

void gc_init(void) {
  if (youngGeneration == NULL) {
    youngGeneration = createHeap(YOUNG_GEN_SIZE);

    if (!youngGeneration) {
      perror("Failed to init GC Heap.");
      exit(1);
    }
  }
}

static void scan_and_collect_garbage(Node *root, Interval *garbageList, int *garbageCount) {
  if (root == NULL) return;

  scan_and_collect_garbage(root->left, garbageList, garbageCount);

  ObjHeader *header = (ObjHeader *)root->i.low;

  if (header->marked == 1) header->marked = 0;
  else {
    if (*garbageCount < MAX_GARBAGE) {
      garbageList[*garbageCount] = root->i;
      (*garbageCount)++;
    }
  }

  scan_and_collect_garbage(root->right, garbageList, garbageCount);
}

void gc_sweep(void) {
  Interval garbage[MAX_GARBAGE];

  int garbageCount = 0;

  scan_and_collect_garbage(global_root, garbage, &garbageCount);

  for (int i = 0; i < garbageCount; i++) removeInterval(&global_root, garbage[i]);

  if (global_root == NULL) resetHeap(youngGeneration);  
}

void gc_collect(void) {
  if (stack_base == NULL) return;
  
  jmp_buf registers;
  setjmp(registers);

  void *stack_top = &registers;

  uintptr_t *current = (uintptr_t *)stack_top;
  uintptr_t *end = (uintptr_t *)stack_base;

  if (current > end) {
    uintptr_t *tmp = current;
    current = end;
    end = tmp;
  }

  while (current < end) {
    uintptr_t pointer = *current;

    Node *node = findPoint(global_root, pointer);

    if (node) {
      ObjHeader *header = (ObjHeader *)node->i.low;

      if (header->marked == 0) header->marked = 1;
    }

    current++;
  }

  gc_sweep();
}

void *gc_malloc(size_t size) {
  if (size == 0) return NULL;

  if (!youngGeneration) gc_init();
  
  size_t totalSize = sizeof(ObjHeader) + size;

  void *mem = allocHeap(youngGeneration, totalSize);

  if (mem == NULL) {
    gc_collect();

    mem = allocHeap(youngGeneration, totalSize);

    if (mem == NULL) {
      perror("Out of Memory. Heap is full.");
      exit(1);
    }
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
