#include "../lib/gc.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#define YOUNG_GEN_SIZE (1024 * 1024)
#define OLD_GEN_SIZE (4 * 1024 * 1024)
#define MAX_GARBAGE 1024

static Heap *youngGeneration = NULL;
static Interval youngIntervals[MAX_GARBAGE];
static int youngCount = 0;

static Heap *oldGeneration = NULL;

Node *global_root = NULL;
void *stack_base = NULL;

void gc_init(void *stack_bottom) {
  stack_base = stack_bottom;

  if (youngGeneration == NULL) {
    youngGeneration = createHeap(YOUNG_GEN_SIZE);

    if (!youngGeneration) {
      perror("Failed to init GC Heap.");
      exit(1);
    }
  }

  if (oldGeneration == NULL) {
    oldGeneration = createHeap(OLD_GEN_SIZE);

    if (!oldGeneration) {
      perror("Failed to init Old Gen Heap.");
      exit(1);
    }
  }
}

static void collect_young_intervals(Node *root) {
  if (root == NULL || youngCount >= MAX_GARBAGE) return;

  collect_young_intervals(root->left);

  ObjHeader *header = (ObjHeader *)root->i.low;

  if (header->generation == 0) youngIntervals[youngCount++] = root->i;

  collect_young_intervals(root->right);
}

void gc_sweep(void) {
  youngCount = 0;
  
  collect_young_intervals(global_root);

  for (int i = 0; i < youngCount; i++) {
    uintptr_t low = youngIntervals[i].low;
    ObjHeader *header = (ObjHeader *)low;
    size_t totalSize = sizeof(ObjHeader) + header->size;

    if (header->generation == 0) {
      void *new_mem = allocHeap(oldGeneration, totalSize);

      if (new_mem == NULL) {
        perror("Out of Memory on Old Generation during evacuation.");
        exit(1);
      }

      memcpy(new_mem, (void *)low, totalSize);

      ObjHeader *new_header = (ObjHeader *)new_mem;
      new_header->generation = 1;
      new_header->marked = 0;

      uintptr_t low = (uintptr_t)new_mem;
      uintptr_t high = low + totalSize;
      Interval new_i = {low, high};

      insert(&global_root, new_i);

      header->generation = -1;
      header->marked = 0;
    } else removeInterval(&global_root, youngIntervals[i]);
  }

  resetHeap(youngGeneration);
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

  if (!youngGeneration) {
    perror("GC was not initialized.");
    exit(1);
  }
  
  size_t totalSize = sizeof(ObjHeader) + size;
  int allocatedGeneration = 0;

  void *mem = allocHeap(youngGeneration, totalSize);

  if (mem == NULL) {
    gc_collect();

    mem = allocHeap(youngGeneration, totalSize);

    if (mem == NULL) {
      mem = allocHeap(oldGeneration, totalSize);
      allocatedGeneration = 1;

      if (mem == NULL) {
        perror("Out of Memory. Heap is full.");
        exit(1);
      }
    }
  }

  ObjHeader *header = (ObjHeader *)mem;
  header->size = size;
  header->generation = allocatedGeneration;
  header->marked = 0;
  header->canary = 0xDEADC0DE;

  void *pointer = (char *)mem + sizeof(ObjHeader);

  uintptr_t low = (uintptr_t)mem;
  uintptr_t high = low + totalSize;
  Interval i = {low, high};

  insert(&global_root, i);

  return pointer;
}
